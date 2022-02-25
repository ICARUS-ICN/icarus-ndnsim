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
 * Author: Pablo Iglesias Sanuy <pabliglesias@alumnos.uvigo.es>
 */

#include "ns3/assert.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/icarus-module.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/mobility-module.h"
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
#include <string>

namespace ns3 {
using namespace icarus;

NS_LOG_COMPONENT_DEFINE ("icarus.Ground2GroundPingExample");

auto
main (int argc, char **argv) -> int
{
  using namespace boost::units;
  using namespace boost::units::si;

  // Track best satellite every minute
  Config::SetDefault ("ns3::icarus::GroundNodeSatTracker::TrackingInterval",
                      TimeValue (Minutes (1)));

  CommandLine cmd;
  cmd.AddValue ("trackingInterval", "ns3::icarus::GroundNodeSatTracker::TrackingInterval");
  cmd.Parse (argc, argv);

  auto n_nodes = (6 * 20) + 2;
  NodeContainer nodes;
  nodes.Create (n_nodes);
  auto ground1 = nodes.Get (n_nodes - 1);
  auto ground2 = nodes.Get (n_nodes - 2);
  NodeContainer birds;
  for (uint32_t i = 0; i < n_nodes - 2; ++i)
    {
      Ptr<Node> p = nodes.Get (i);
      birds.Add (p);
    }

  IcarusHelper icarusHelper;
  ISLHelper islHelper;
  ConstellationHelper constellationHelper (quantity<length> (250 * kilo * meters),
                                           quantity<plane_angle> (60 * degree::degree), 6, 20, 1);

  ObjectFactory staticPositionsFactory ("ns3::ListPositionAllocator");
  auto staticPositions = staticPositionsFactory.Create<ListPositionAllocator> ();
  staticPositions->Add (GeographicPositions::GeographicToCartesianCoordinates (
      42.1704632, -8.6877909, 450, GeographicPositions::WGS84)); // Our School
  staticPositions->Add (GeographicPositions::GeographicToCartesianCoordinates (
      40.712742, -74.013382, -17, GeographicPositions::WGS84)); // World Trade Center, NYC
  MobilityHelper staticHelper;
  staticHelper.SetPositionAllocator (staticPositions);
  staticHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  staticHelper.Install (ground1);
  staticHelper.Install (ground2);
  icarusHelper.Install (nodes, constellationHelper);

  islHelper.Install (birds, constellationHelper);

  ns3::Simulator::Stop (Days (7));

  ns3::Simulator::Run ();
  ns3::Simulator::Destroy ();

  return 0;
}
} // namespace ns3

auto
main (int argc, char **argv) -> int
{
  return ns3::main (argc, argv);
}