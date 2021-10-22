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

#include "ns3/drop-tail-queue.h"
#include "ns3/icarus-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/core-module.h"
#include "ns3/icarus-module.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/net-device.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/udp-echo-helper.h"
#include "src/core/model/log.h"

#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/angle/degrees.hpp>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SputPingExample");

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

  ObjectFactory circularOrbitFactory ("ns3::CircularOrbitMobilityModel");

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

  IcarusHelper icarusHelper;
  NetDeviceContainer netDevices = icarusHelper.Install (nodes);

  InternetStackHelper ipStack;
  ipStack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer ipInterfaces;
  ipInterfaces = address.Assign (netDevices);

  UdpEchoClientHelper echoClient (ipInterfaces.GetAddress (0), 7667);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (10000));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1280));

  ApplicationContainer clientApps = echoClient.Install (ground);

  UdpEchoServerHelper echoServer (7667);
  ApplicationContainer serverApps = echoServer.Install (bird);

  clientApps.Start (Seconds (0.0));

  LogComponentEnable ("UdpEchoServerApplication", ns3::LOG_INFO);

  ns3::Simulator::Stop (Days (7));

  ns3::Simulator::Run ();
  ns3::Simulator::Destroy ();

  return 0;
}