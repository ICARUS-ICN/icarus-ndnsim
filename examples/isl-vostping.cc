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

#include "ns3/core-module.h"
#include "ns3/isl-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/icarus-module.h"
#include "ns3/trace-helper.h"
#include "ns3/ndnSIM-module.h"

#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/si/prefixes.hpp>

namespace ns3 {
using namespace icarus;

NS_LOG_COMPONENT_DEFINE ("icarus.ISLVostPingExample");

auto
main (int argc, char **argv) -> int
{
  using boost::units::quantity;
  using boost::units::degree::degrees;
  using boost::units::si::kilo;
  using boost::units::si::length;
  using boost::units::si::meters;
  using boost::units::si::plane_angle;

  CommandLine cmd;

  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);
  auto bird1 = nodes.Get (0);
  auto bird2 = nodes.Get (1);

  IcarusHelper icarusHelper;
  ISLHelper islHelper;
  ConstellationHelper constellationHelper (quantity<length> (250 * kilo * meters),
                                           quantity<plane_angle> (60.0 * degrees), 2, 1, 0);

  ObjectFactory circularOrbitFactory ("ns3::icarus::CircularOrbitMobilityModel");

  NetDeviceContainer netDevices (icarusHelper.Install (nodes, &constellationHelper));
  netDevices.Add(islHelper.Install(nodes));

  // Install NDN stack on all nodes
  ns3::ndn::StackHelper ndnHelper;
  IcarusHelper::FixNdnStackHelper (ndnHelper);
  ndnHelper.SetDefaultRoutes (true);
  ndnHelper.InstallAll ();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll ("/icarus", "/localhost/nfd/strategy/best-route");

  // Insert routes
  auto proto = bird1 -> GetObject<ndn::L3Protocol>();
  auto face = proto ->getFaceByNetDevice(bird1->GetDevice(1));

  ndn::FibHelper::AddRoute (bird1,"/icarus",face, int32_t(1));
  
  // Installing applications
  
  // Consumer
  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix ("/icarus/bird1/isl-vostping");
  consumerHelper.SetAttribute ("Frequency", StringValue ("1")); // 1 interests a second
  auto apps = consumerHelper.Install (bird1);

  // Producer
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix ("/icarus/bird1/isl-vostping");
  producerHelper.SetAttribute ("PayloadSize", StringValue ("1024"));
  producerHelper.Install (bird2);

  AsciiTraceHelper ascii;
  auto stream = ascii.CreateFileStream ("/tmp/out.tr");
  stream->GetStream ()->precision (9);
  islHelper.EnableAscii (stream,0,1);
  islHelper.EnableAscii (stream, 1,1);
  islHelper.EnablePcap ("/tmp/pcap-isl-vostping-1",0,1);
  islHelper.EnablePcap("/tmp/pcap-isl-vostping-2",1,1);

  // Add channel drops to the ASCII trace
  Config::Connect ("/NodeList/0/DeviceList/1/$ns3::icarus::SatNetDevice/Channel/PhyTxDrop",
                   MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));

  ns3::Simulator::Stop (Days (7));

  ns3::Simulator::Run ();
  ns3::Simulator::Destroy ();

  return 0;
}
} // namespace ns3

auto
main (int argc, char *argv[]) -> int
{
  return ns3::main (argc, argv);
}
