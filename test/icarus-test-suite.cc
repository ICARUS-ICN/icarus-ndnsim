/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Universidade de Vigo
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

// An essential include is test.h
#include "ns3/icarus-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/object-factory.h"
#include "ns3/object.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

#include "ns3/node.h"
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/si/prefixes.hpp>

// Do not put your test classes in namespace ns3.  You may find it useful
// to use the using directive to access the ns3 namespace directly
using namespace ns3;
using namespace icarus;
// This is an example TestCase.
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

class ISLGridTestCase : public TestCase
{
public:
  ISLGridTestCase ();
  virtual ~ISLGridTestCase ();

private:
  virtual void DoRun (void);
};

ISLGridTestCase::ISLGridTestCase () : TestCase ("Check ISL grid link formation")
{
}

ISLGridTestCase::~ISLGridTestCase ()
{
}

void
ISLGridTestCase::DoRun (void)
{

  using namespace boost::units;
  using namespace boost::units::si;
  using boost::units::si::kilo_type;

  IcarusHelper icarusHelper;
  ISLHelper islHelper;
  ConstellationHelper constellationHelper (quantity<length> (250 * kilo * meters),
                                           quantity<plane_angle> (60 * degree::degree), 6, 20, 1);

  NodeContainer nodes;
  nodes.Create (6 * 20);
  icarusHelper.Install (nodes, &constellationHelper);
  islHelper.Install (nodes, &constellationHelper);
  const auto &constellation = constellationHelper.GetConstellation ();

  for (std::size_t plane = 0; plane < constellation->GetNPlanes (); plane++)
    {
      for (std::size_t index = 0; index < constellation->GetPlaneSize (); index++)
        {
          const auto &sat = constellation->GetSatellite (plane, index);
          const auto nlinks = sat->GetNode ()->GetNDevices ();
          NS_TEST_ASSERT_MSG_EQ (nlinks, 5, "Number of links is wrong!");
        }
    }

  ns3::Simulator::Stop (Seconds (2));

  ns3::Simulator::Run ();
}

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
  // TestDuration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER
  AddTestCase (new CircularOrbitTestCase1, TestCase::QUICK);
  AddTestCase (new ISLGridTestCase, TestCase::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
static IcarusTestSuite icarusTestSuite;
