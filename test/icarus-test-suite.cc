/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021–2022 Universidade de Vigo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Miguel Rodríguez Pérez <miguel@det.uvigo.gal>
 */

// Include a header file from your module to test.
#include "ns3/circular-orbit.h"
#include "model/orbit/satpos/planet.h"

// An essential include is test.h
#include "ns3/constant-position-mobility-model.h"
#include "ns3/geographic-positions.h"
#include "ns3/icarus-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/object-factory.h"
#include "ns3/object.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

#include "ns3/node.h"
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/time.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/si/prefixes.hpp>
#include <ios>
#include <sstream>

// Do not put your test classes in namespace ns3.  You may find it useful
// to use the using directive to access the ns3 namespace directly
using namespace ns3;
using namespace ns3::icarus;
class CircularOrbitTestCase1 : public TestCase
{
public:
  CircularOrbitTestCase1 ();
  virtual ~CircularOrbitTestCase1 ();

private:
  virtual void DoRun (void);

  ObjectFactory m_circularOrbit;
};

// Add some help text to this case to describe what it is intended to test
CircularOrbitTestCase1::CircularOrbitTestCase1 () : TestCase ("Check orbit calculation")
{
}

// This destructor does nothing but we include it as a reminder that
// the test case should clean up after itself
CircularOrbitTestCase1::~CircularOrbitTestCase1 ()
{
}

//
// This method is the pure virtual method from class TestCase that every
// TestCase must implement
//
void
CircularOrbitTestCase1::DoRun (void)
{
  using boost::units::quantity;
  using boost::units::degree::degrees;
  using boost::units::si::meters;
  using boost::units::si::plane_angle;
  using boost::units::si::radians;

  // set types
  m_circularOrbit.SetTypeId ("ns3::icarus::CircularOrbitMobilityModel");

  Ptr<Node> node = CreateObject<Node> ();

  Ptr<CircularOrbitMobilityModel> mmodel = m_circularOrbit.Create<CircularOrbitMobilityModel> ();
  mmodel->LaunchSat (quantity<plane_angle> (60.0 * degrees), 0.0 * radians, 250e3 * meters,
                     0.0 * radians);
  node->AggregateObject (mmodel);

  NS_TEST_ASSERT_MSG_EQ_TOL (sqrt (pow (mmodel->GetPosition ().x, 2) +
                                   pow (mmodel->GetPosition ().y, 2) +
                                   pow (mmodel->GetPosition ().z, 2)),
                             6621000., 1, "Initial altitude is wrong!");

  ns3::Simulator::Stop (Seconds (9384)); // Bird at maximum inclination South
  ns3::Simulator::Run ();

  NS_TEST_ASSERT_MSG_EQ_TOL (sqrt (pow (mmodel->GetPosition ().x, 2) +
                                   pow (mmodel->GetPosition ().y, 2) +
                                   pow (mmodel->GetPosition ().z, 2)),
                             6621000., 1, "Final altitude is wrong!");
  NS_TEST_ASSERT_MSG_EQ_TOL (mmodel->GetPosition ().x, -2.09605e6, 1000, "Position is wrong");
}

class CircularOrbitElevationTest : public TestCase
{
public:
  CircularOrbitElevationTest (boost::units::quantity<boost::units::si::length> altitude,
                              boost::units::quantity<boost::units::si::plane_angle> elevation,
                              boost::units::quantity<boost::units::si::length> expected_distance)
      : TestCase ("Circular Orbit Elevation Test"),
        elevation (elevation),
        distance (expected_distance)
  {
    ObjectFactory m_circularOrbit;

    m_circularOrbit.SetTypeId ("ns3::icarus::CircularOrbitMobilityModel");
    mmodel = m_circularOrbit.Create<CircularOrbitMobilityModel> ();
    mmodel->LaunchSat (0 * boost::units::si::radians, 0 * boost::units::si::radians, altitude,
                       0 * boost::units::si::radians);
  }
  virtual ~CircularOrbitElevationTest () = default;

private:
  const boost::units::quantity<boost::units::si::plane_angle> elevation;
  const boost::units::quantity<boost::units::si::length> distance;
  virtual void
  DoRun (void)
  {
    NS_TEST_ASSERT_MSG_EQ_TOL (
        mmodel->getGroundDistanceAtElevation (
            elevation, ::icarus::satpos::planet::constants::Earth.getRadius ()),
        distance.value (), 1000, "Distance is way off");
  }

  Ptr<CircularOrbitMobilityModel> mmodel;
};

class ISLGridTestCase1 : public TestCase
{
public:
  ISLGridTestCase1 (std::size_t n_planes, std::size_t n_satellites_per_plane,
                    std::size_t expected_links);
  virtual ~ISLGridTestCase1 () override = default;

private:
  const std::size_t m_NPlanes;
  const std::size_t m_NSatellitesPerPlane;
  const std::size_t m_NExpectedLinks;

  virtual void DoRun (void) override;
};

namespace {
std::string
GetTestName (std::size_t planes, std::size_t plane_size) noexcept
{
  std::ostringstream name ("Check ISL grid link formation: ");
  name << planes << "×" << plane_size;

  return name.str ();
}
} // namespace

ISLGridTestCase1::ISLGridTestCase1 (std::size_t n_planes, std::size_t n_satellites_per_plane,
                                    std::size_t expected_links)
    : TestCase (GetTestName (n_planes, n_satellites_per_plane)),
      m_NPlanes (n_planes),
      m_NSatellitesPerPlane (n_satellites_per_plane),
      m_NExpectedLinks (expected_links)
{
}

void
ISLGridTestCase1::DoRun (void)
{

  using namespace boost::units;
  using namespace boost::units::si;
  using boost::units::si::kilo_type;

  IcarusHelper icarusHelper;
  ISLHelper islHelper;
  ConstellationHelper constellationHelper (quantity<length> (250 * kilo * meters),
                                           quantity<plane_angle> (60 * degree::degree), m_NPlanes,
                                           m_NSatellitesPerPlane, 1);

  NodeContainer nodes;
  nodes.Create (m_NPlanes * m_NSatellitesPerPlane);
  icarusHelper.Install (nodes, constellationHelper);
  islHelper.Install (nodes, constellationHelper);
  const auto &constellation = constellationHelper.GetConstellation ();

  for (std::size_t plane = 0; plane < constellation->GetNPlanes (); plane++)
    {
      for (std::size_t index = 0; index < constellation->GetPlaneSize (); index++)
        {
          const auto sat = constellation->GetSatellite (plane, index);
          const auto nlinks = sat->GetNode ()->GetNDevices ();
          NS_TEST_ASSERT_MSG_EQ (nlinks, m_NExpectedLinks, "Number of links is wrong!");
        }
    }

  ns3::Simulator::Stop (Seconds (2));

  ns3::Simulator::Run ();
}

class FindNextPassTest : public TestCase
{
  using length = boost::units::quantity<boost::units::si::length>;
  using plane_angle = boost::units::quantity<boost::units::si::plane_angle>;
  using time = boost::units::quantity<boost::units::si::time>;

public:
  FindNextPassTest (length altitude, plane_angle elevation, plane_angle inclination,
                    plane_angle ascending_node, plane_angle latitude, plane_angle longitude,
                    time expected_time)
      : TestCase ("Next Pass Test"), expected_time (expected_time), elevation (elevation)
  {
    ObjectFactory circularOrbit, staticPosition;

    circularOrbit.SetTypeId ("ns3::icarus::CircularOrbitMobilityModel");
    staticPosition.SetTypeId ("ns3::ConstantPositionMobilityModel");
    sat = circularOrbit.Create<CircularOrbitMobilityModel> ();
    sat->LaunchSat (inclination, ascending_node, altitude, 0 * boost::units::si::radians);
    auto static_pos = staticPosition.Create<ConstantPositionMobilityModel> ();
    static_pos->SetPosition (GeographicPositions::GeographicToCartesianCoordinates (
        boost::units::quantity<boost::units::degree::plane_angle> (latitude).value (),
        boost::units::quantity<boost::units::degree::plane_angle> (longitude).value (), 0,
        GeographicPositions::WGS84));

    ground = CreateObject<Node> ();
    ground->AggregateObject (static_pos);
  }
  virtual ~FindNextPassTest () = default;

private:
  Ptr<CircularOrbitMobilityModel> sat;
  Ptr<Node> ground;
  const time expected_time;
  const plane_angle elevation;

  virtual void
  DoRun (void)
  {
    auto encounter = sat->getNextTimeAtElevation (elevation, ground);
    NS_TEST_ASSERT_MSG_EQ_TOL (encounter.GetSeconds (), expected_time.value (), 10,
                               "Time is way off");
  }
};

// The TestSuite class names the TestSuite, identifies what type of TestSuite,
// and enables the TestCases to be run.  Typically, only the constructor for
// this class must be defined
//
class IcarusTestSuite : public TestSuite
{
public:
  IcarusTestSuite ();
};

IcarusTestSuite::IcarusTestSuite () : TestSuite ("icarus", UNIT)
{
  using boost::units::quantity;
  using boost::units::degree::degrees;
  using boost::units::si::kilo;
  using boost::units::si::length;
  using boost::units::si::meter;
  using boost::units::si::plane_angle;

  // TestDuration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER

  AddTestCase (new FindNextPassTest (
                   quantity<length> (400 * kilo * meter), quantity<plane_angle> (25 * degrees),
                   quantity<plane_angle> (45 * degrees), 0 * boost::units::si::radians,
                   quantity<plane_angle> (30 * degrees), quantity<plane_angle> (0 * degrees),
                   6075.39 * boost::units::si::seconds),
               TestCase::QUICK);

  AddTestCase (new CircularOrbitTestCase1, TestCase::QUICK);
  AddTestCase (new CircularOrbitElevationTest (quantity<length> (400 * kilo * meter),
                                               quantity<plane_angle> (10 * degrees),
                                               1439415 * meter),
               TestCase::QUICK);
  AddTestCase (new CircularOrbitElevationTest (quantity<length> (400 * kilo * meter),
                                               quantity<plane_angle> (25 * degrees),
                                               843933 * meter),
               TestCase::QUICK);
  AddTestCase (new CircularOrbitElevationTest (quantity<length> (400 * kilo * meter),
                                               quantity<plane_angle> (45 * degrees),
                                               549884 * meter),
               TestCase::QUICK);
  AddTestCase (new CircularOrbitElevationTest (quantity<length> (400 * kilo * meter),
                                               quantity<plane_angle> (60 * degrees),
                                               457419 * meter),
               TestCase::QUICK);
  AddTestCase (new CircularOrbitElevationTest (quantity<length> (400 * kilo * meter),
                                               quantity<plane_angle> (90 * degrees),
                                               400000 * meter),
               TestCase::QUICK);
  AddTestCase (new ISLGridTestCase1 (6, 20, 5), TestCase::QUICK);
  AddTestCase (new ISLGridTestCase1 (1, 1, 1), TestCase::QUICK);
  AddTestCase (new ISLGridTestCase1 (1, 2, 2), TestCase::QUICK);
  AddTestCase (new ISLGridTestCase1 (2, 1, 2), TestCase::QUICK);
  AddTestCase (new ISLGridTestCase1 (2, 2, 3), TestCase::QUICK);
  AddTestCase (new ISLGridTestCase1 (3, 2, 4), TestCase::QUICK);
  AddTestCase (new ISLGridTestCase1 (2, 3, 4), TestCase::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
static IcarusTestSuite icarusTestSuite;
