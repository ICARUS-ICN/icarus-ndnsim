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

#include "ns3/address.h"
#include "ns3/arp-cache.h"
#include "ns3/callback.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/ground-sat-channel.h"
#include "ns3/icarus-helper.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/mobility-module.h"
#include "ns3/core-module.h"
#include "ns3/icarus-module.h"
#include "ns3/net-device-container.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/net-device.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/pointer.h"
#include "ns3/trace-helper.h"
#include "ns3/udp-echo-helper.h"
#include "src/core/model/log.h"
#include "ns3/sat-address.h"

#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/si/prefixes.hpp>
#include <limits>

using namespace ns3;
using namespace icarus;

NS_LOG_COMPONENT_DEFINE ("icarus.SputPingExample");

auto
main (int argc, char **argv) -> int
{
  using boost::units::quantity;
  using boost::units::degree::degrees;
  using boost::units::si::kilo;
  using boost::units::si::length;
  using boost::units::si::meters;
  using boost::units::si::plane_angle;
  using boost::units::si::radians;
  CommandLine cmd;

  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (4);
  auto ground1 = nodes.Get (0);
  auto ground2 = nodes.Get (1);

  IcarusHelper icarusHelper;
  ConstellationHelper chelper (quantity<length> (250 * kilo * meters),
                               quantity<plane_angle> (60.0 * degrees), 2, 1, 0);
  auto bird1 = nodes.Get (2);
  auto bird2 = nodes.Get (3);

  ObjectFactory staticPositionsFactory ("ns3::ListPositionAllocator");
  auto staticPositions = staticPositionsFactory.Create<ListPositionAllocator> ();
  staticPositions->Add (GeographicPositions::GeographicToCartesianCoordinates (
      42.1704632, -8.6877909, 450, GeographicPositions::WGS84)); // Our School
  staticPositions->Add (GeographicPositions::GeographicToCartesianCoordinates (
      -27.39979, -33.66416, 0, GeographicPositions::WGS84)); // Somewhere else
  MobilityHelper staticHelper;
  staticHelper.SetPositionAllocator (staticPositions);
  staticHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  staticHelper.Install (ground1);
  staticHelper.Install (ground2);

  NetDeviceContainer netDevices (icarusHelper.Install (nodes, &chelper));

  InternetStackHelper ipStack;
  ipStack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer ipInterfaces;
  ipInterfaces = address.Assign (netDevices);

  UdpEchoClientHelper echoClient (ipInterfaces.GetAddress (2), 7667);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (std::numeric_limits<uint32_t>::max ()));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1280));
  ApplicationContainer clientApps = echoClient.Install (ground1);

  echoClient.SetAttribute ("RemoteAddress", AddressValue (ipInterfaces.GetAddress (3)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1400));
  clientApps.Add (echoClient.Install (ground2));

  DynamicCast<GroundStaNetDevice> (ground1->GetDevice (0))
      ->SetRemoteAddress (bird1->GetDevice (0)->GetAddress ());
  DynamicCast<GroundStaNetDevice> (ground2->GetDevice (0))
      ->SetRemoteAddress (bird2->GetDevice (0)->GetAddress ());

  UdpEchoServerHelper echoServer (7667);
  ApplicationContainer serverApps = echoServer.Install (bird1);
  serverApps.Add (echoServer.Install (bird2));

  clientApps.Start (Seconds (0.0));

  LogComponentEnable ("UdpEchoServerApplication", ns3::LOG_INFO);

  AsciiTraceHelper ascii;
  auto stream = ascii.CreateFileStream ("/tmp/out.tr");
  stream->GetStream ()->precision (9);
  icarusHelper.EnableAsciiAll (stream);
  icarusHelper.EnablePcapAll ("/tmp/pcap-sputping.pcap");

  // Add channel drops to the ASCII trace
  Config::Connect ("/NodeList/0/DeviceList/0/$ns3::icarus::IcarusNetDevice/Channel/PhyTxDrop",
                   MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));

  ns3::Simulator::Stop (Days (7));

  ns3::Simulator::Run ();
  ns3::Simulator::Destroy ();

  return 0;
}