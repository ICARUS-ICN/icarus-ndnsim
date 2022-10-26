/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2022 Universidade de Vigo
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

#include "ns3/command-line.h"
#include "ns3/ground-sta-net-device.h"
#include "ns3/log.h"
#include "ns3/ptr.h"
#include "ns3/sat-address.h"
#include "ns3/simulator.h"
#include "ns3/geographic-positions.h"
#include "ns3/mobility-helper.h"
#include "ns3/icarus-helper.h"
#include "ns3/ground-node-sat-tracker-elevation.h"
#include <boost/units/systems/si/prefixes.hpp>

NS_LOG_COMPONENT_DEFINE ("icarus.ConstellationTrackerExample");

namespace ns3 {
namespace icarus {

using namespace boost::units;
using namespace boost::units::si;

auto
main (int argc, char **argv) -> int
{
  std::size_t n_planes = 60;
  std::size_t n_satellites_per_plane = 42;
  double latitude = 0;
  double inclination = 60;
  int altitude = 400;
  ns3::Time duration = Days (7);

  CommandLine cmd;

  cmd.AddValue ("duration", "Simulation duration", duration);
  cmd.AddValue ("planes", "Number of planes", n_planes);
  cmd.AddValue ("satplane", "Number of satellites per plane", n_satellites_per_plane);
  cmd.AddValue ("latitude", "Latitude of ground station", latitude);
  cmd.AddValue ("inclination", "Orbit inclination, in degrees", inclination);
  cmd.AddValue ("altitude", "Orbit altitude, in km", altitude);

  cmd.Parse (argc, argv);

  IcarusHelper icarusHelper;
  icarusHelper.SetTrackerModel ("ns3::icarus::GroundNodeSatTrackerElevation");
  ConstellationHelper constellationHelper (quantity<length> (altitude * kilo * meters),
                                           quantity<plane_angle> (inclination * degree::degree),
                                           n_planes, n_satellites_per_plane, 1);

  NodeContainer sat_nodes;
  sat_nodes.Create (n_planes * n_satellites_per_plane);

  NodeContainer ground_nodes;
  ground_nodes.Create (1);

  ObjectFactory staticPositionsFactory ("ns3::ListPositionAllocator");
  auto staticPositions = staticPositionsFactory.Create<ListPositionAllocator> ();
  staticPositions->Add (GeographicPositions::GeographicToCartesianCoordinates (
      latitude, 0, 0, GeographicPositions::WGS84)); // Equator at Greenwich
  MobilityHelper staticHelper;
  staticHelper.SetPositionAllocator (staticPositions);
  staticHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  staticHelper.Install (ground_nodes.Get (0));

  NodeContainer all_nodes = sat_nodes;
  all_nodes.Add (ground_nodes);

  icarusHelper.Install (all_nodes, constellationHelper);

  DynamicCast<GroundStaNetDevice> (ground_nodes.Get (0)->GetDevice (0))
      ->remoteAddressChange.connect (std::bind (
          [=] (const auto &_oldAddress, const auto &newAddress, double latitude) {
            static auto last_time = Seconds (0);

            std::cerr << Simulator::Now ().GetSeconds () << "s " << latitude << "° tracking ("
                      << newAddress.getOrbitalPlane () << ", " << newAddress.getPlaneIndex ()
                      << ") for " << (ns3::Simulator::Now () - last_time).GetSeconds () << "s."
                      << std::endl;

            last_time = ns3::Simulator::Now ();
          },
          _1, _2, latitude));

  ground_nodes.Get (0)->GetObject<GroundNodeSatTrackerElevation> ()->satsAvailable.connect (
      [] (const auto &sats) {
        for (const auto &sat_info : sats)
          {
            Time avail;
            std::size_t plane, index;

            std::tie (avail, plane, index) = sat_info;
            std::cerr << "\t(" << plane << ", " << index << ") for " << avail.GetSeconds () << "s."
                      << std::endl;
          }
      });

  ns3::Simulator::Stop (duration);

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
