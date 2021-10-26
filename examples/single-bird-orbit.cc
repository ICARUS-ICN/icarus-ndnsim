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
#include "ns3/mobility-model.h"
#include "ns3/object-factory.h"
#include "ns3/object.h"
#include "ns3/simulator.h"
#include "ns3/geographic-positions.h"

#include "ns3/node.h"
#include "src/core/model/nstime.h"
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/angle/degrees.hpp>

// Do not put your test classes in namespace ns3.  You may find it useful
// to use the using directive to access the ns3 namespace directly
using namespace ns3;
using namespace icarus;
// This is an example TestCase.
class SingleOrbitTest
{
public:
  SingleOrbitTest ();
  virtual ~SingleOrbitTest ();
  bool DoRun ();
};

SingleOrbitTest::SingleOrbitTest ()
{
}

SingleOrbitTest::~SingleOrbitTest ()
{
}

namespace {

constexpr double EARTH_RADIUS = 6371.009e3;
constexpr double EARTH_SEMIMAJOR_AXIS = 6378137;
constexpr double EARTH_GRS80_ECCENTRICITY = 0.0818191910428158;
constexpr double EARTH_WGS84_ECCENTRICITY = 0.0818191908426215;
constexpr double DEG2RAD = M_PI / 180.0;
constexpr double RAD2DEG = 180.0 * M_1_PI;
// ndnSIM version of ns-3 lacks this method. Copy it here.
Vector
CartesianToGeographicCoordinates (Vector pos, GeographicPositions::EarthSpheroidType sphType)
{
  double a; // semi-major axis of earth
  double e; // first eccentricity of earth
  if (sphType == GeographicPositions::SPHERE)
    {
      a = EARTH_RADIUS;
      e = 0;
    }
  else if (sphType == GeographicPositions::GRS80)
    {
      a = EARTH_SEMIMAJOR_AXIS;
      e = EARTH_GRS80_ECCENTRICITY;
    }
  else // if sphType == WGS84
    {
      a = EARTH_SEMIMAJOR_AXIS;
      e = EARTH_WGS84_ECCENTRICITY;
    }

  Vector lla, tmp;
  lla.y = atan2 (pos.y, pos.x); // longitude (rad), in +/- pi

  double e2 = e * e;
  // sqrt (pos.x^2 + pos.y^2)
  double p = CalculateDistance (pos, {0, 0, pos.z});
  lla.x = atan2 (pos.z, p * (1 - e2)); // init latitude (rad), in +/- pi

  do
    {
      tmp = lla;
      double N = a / sqrt (1 - e2 * sin (tmp.x) * sin (tmp.x));
      double v = p / cos (tmp.x);
      lla.z = v - N; // altitude
      lla.x = atan2 (pos.z, p * (1 - e2 * N / v));
  }
  // 1 m difference is approx 1 / 30 arc seconds = 9.26e-6 deg
  while (fabs (lla.x - tmp.x) > 0.00000926 * DEG2RAD);

  lla.x *= RAD2DEG;
  lla.y *= RAD2DEG;

  // canonicalize (latitude) x in [-90, 90] and (longitude) y in [-180, 180)
  if (lla.x > 90.0)
    {
      lla.x = 180 - lla.x;
      lla.y += lla.y < 0 ? 180 : -180;
    }
  else if (lla.x < -90.0)
    {
      lla.x = -180 - lla.x;
      lla.y += lla.y < 0 ? 180 : -180;
    }
  if (lla.y == 180.0)
    lla.y = -180;

  // make sure lat/lon in the right range to double check canonicalization
  // and conversion routine
  NS_ASSERT_MSG (-180.0 <= lla.y, "Conversion error: longitude too negative");
  NS_ASSERT_MSG (180.0 > lla.y, "Conversion error: longitude too positive");
  NS_ASSERT_MSG (-90.0 <= lla.x, "Conversion error: latitude too negative");
  NS_ASSERT_MSG (90.0 >= lla.x, "Conversion error: latitude too positive");

  return lla;
}

void
dumpLocation (const Ptr<Node> &node)
{
  std::cout << ns3::Simulator::Now ().GetSeconds () << ":\t";

  const auto location = node->GetObject<MobilityModel> ()->GetPosition ();
  const auto geolocation = CartesianToGeographicCoordinates (location, GeographicPositions::WGS84);

  std::cout << geolocation.y << ", " << geolocation.x << "\t(" << location.x << ", " << location.y
            << ", " << location.z << ')' << std::endl;

  ns3::Simulator::Schedule (Seconds (1), dumpLocation, node);
}
} // namespace

//
// This method is the pure virtual method from class TestCase that every
// TestCase must implement
//
auto
SingleOrbitTest::DoRun (void) -> bool
{
  using boost::units::quantity;
  using boost::units::degree::degrees;
  using boost::units::si::meters;
  using boost::units::si::plane_angle;
  using boost::units::si::radians;

  // set types
  ObjectFactory circularOrbitFactory;

  circularOrbitFactory.SetTypeId ("ns3::icarus::CircularOrbitMobilityModel");

  Ptr<Node> node = CreateObject<Node> ();

  Ptr<CircularOrbitMobilityModel> mmodel =
      circularOrbitFactory.Create<CircularOrbitMobilityModel> ();
  mmodel->LaunchSat (quantity<plane_angle> (60.0 * degrees), 0.0 * radians, 250e3 * meters,
                     0.0 * radians);
  node->AggregateObject (mmodel);

  ns3::Simulator::Stop (Seconds (10296.2)); // Bird at maximum inclination

  ns3::Simulator::Schedule (Seconds (0.0), dumpLocation, node);

  ns3::Simulator::Run ();
  ns3::Simulator::Destroy ();

  return true;
}

auto
main (int argc, const char **argv) -> int
{
  SingleOrbitTest example;

  example.DoRun ();
  return 0;
}