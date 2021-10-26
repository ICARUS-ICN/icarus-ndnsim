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

#include "ns3/circular-orbit.h"

#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/node-container.h"
#include "ns3/position-allocator.h"
#include "ns3/simulator.h"

#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/command-line.h"
#include "ns3/mobility-module.h"
#include "src/core/model/vector.h"

#include <algorithm>
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/angle/degrees.hpp>
#include <limits>

using namespace ns3;
using namespace icarus;

NS_LOG_COMPONENT_DEFINE ("icarus.IntermittentConnectionExample");

namespace {
void
showDistance (const Ptr<Node> &node1, const Ptr<Node> &node2)
{
  static auto minimum = std::numeric_limits<double>::infinity ();
  static auto maximum = -std::numeric_limits<double>::infinity ();
  const auto pos1 = node1->GetObject<MobilityModel> ()->GetPosition ();
  const auto pos2 = node2->GetObject<MobilityModel> ()->GetPosition ();

  const auto distance = CalculateDistance (pos1, pos2);
  minimum = std::min (minimum, distance);
  maximum = std::max (maximum, distance);

  NS_LOG (ns3::LOG_INFO, "Distance from bird to ground " << distance / 1000. << " km."
                                                         << " Maximum: " << maximum / 1000.
                                                         << " km. Minimum: " << minimum / 1000.
                                                         << " km.");

  ns3::Simulator::Schedule (Seconds (1), showDistance, node1, node2);
}
} // namespace

auto
main (int argc, char **argv) -> int
{
  using boost::units::quantity;
  using boost::units::degree::degrees;
  using boost::units::si::meters;
  using boost::units::si::plane_angle;
  using boost::units::si::radians;
  CommandLine cmd;

  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);
  auto bird = nodes.Get (0);
  auto ground = nodes.Get (1);

  ObjectFactory circularOrbitFactory ("ns3::icarus::CircularOrbitMobilityModel");

  auto mmodel = circularOrbitFactory.Create<CircularOrbitMobilityModel> ();
  mmodel->LaunchSat (quantity<plane_angle> (60.0 * degrees), 0.0 * radians, 250e3 * meters,
                     0.0 * radians);
  bird->AggregateObject (mmodel);

  ObjectFactory staticPositionsFactory ("ns3::ListPositionAllocator");
  auto staticPositions = staticPositionsFactory.Create<ListPositionAllocator> ();
  staticPositions->Add (GeographicPositions::GeographicToCartesianCoordinates (
      42.1704632, -8.6877909, 450, GeographicPositions::WGS84)); // Our School
  MobilityHelper staticHelper;
  staticHelper.SetPositionAllocator (staticPositions);
  staticHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  staticHelper.Install (ground);

  ns3::Simulator::Stop (Days (7));

  ns3::Simulator::Schedule (Seconds (0.0), showDistance, bird, ground);

  ns3::Simulator::Run ();
  ns3::Simulator::Destroy ();

  return 0;
}