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

#include "ns3/assert.h"
#include "ns3/command-line.h"
#include "ns3/icarus-module.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/node-container.h"
#include "ns3/simulator.h"
#include "ns3/geographic-positions.h"
#include "src/icarus/helper/isl-helper.h"

#include <boost/units/io.hpp>
#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/si/prefixes.hpp>
#include <boost/units/systems/si/length.hpp>
#include <cstddef>

NS_LOG_COMPONENT_DEFINE ("icarus.ISLGridExample");

namespace ns3 {
namespace icarus {

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
logLinks (const Ptr<Constellation> &constellation)
{
  NS_LOG_FUNCTION (constellation);

  for (std::size_t plane = 0; plane < constellation->GetNPlanes (); plane++)
    {
      for (std::size_t index = 0; index < constellation->GetPlaneSize (); index++)
        {
          const auto &sat = constellation->GetSatellite (plane, index);
          const auto nlinks = sat->GetNode ()->GetNDevices ();
          NS_LOG_DEBUG ("Satellite: ("
                        << plane << ", " << index << ") has " << nlinks
                        << " links");
        }
    }

  ns3::Simulator::Schedule (Seconds (1.0), logLinks, constellation);
}
} // namespace

auto
main (int argc, char **argv) -> int
{
  using namespace boost::units;
  using namespace boost::units::si;

  CommandLine cmd;

  cmd.Parse (argc, argv);

  IcarusHelper icarusHelper;
  ISLHelper islHelper;
  ConstellationHelper constellationHelper (quantity<length> (250 * kilo * meters),
                                           quantity<plane_angle> (60 * degree::degree), 6, 20, 1);

  NodeContainer nodes;
  nodes.Create (6 * 20);
  icarusHelper.Install (nodes, &constellationHelper);
  islHelper.Install (nodes, &constellationHelper);

  ns3::Simulator::Stop (Days (7));

  ns3::Simulator::Schedule (Seconds (0.0), logLinks, constellationHelper.GetConstellation ());

  ns3::Simulator::Run ();
  ns3::Simulator::Destroy ();

  return 0;
}
} // namespace icarus
} // namespace ns3

auto
main (int argc, char **argv) -> int
{
  return ns3::icarus::main (argc, argv);
}