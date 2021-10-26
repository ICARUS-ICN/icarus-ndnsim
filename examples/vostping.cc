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

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/icarus-module.h"
#include "ns3/trace-helper.h"
#include "ns3/log.h"
#include "ns3/ndnSIM-module.h"

#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/angle/degrees.hpp>
#include <limits>

namespace ns3 {
using namespace icarus;

NS_LOG_COMPONENT_DEFINE ("icarus.VostPingExample");

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

  IcarusHelper icarusHelper;
  NetDeviceContainer netDevices = icarusHelper.Install (nodes);

  // Install NDN stack on all nodes
  ns3::ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes (true);
  ndnHelper.InstallAll ();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll ("/icarus", "/localhost/nfd/strategy/best-route");

  // Installing applications

  // Consumer
  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix ("/icarus/bird1/vostping");
  consumerHelper.SetAttribute ("Frequency", StringValue ("1")); // 1 interests a second
  auto apps = consumerHelper.Install (ground); // ground node

  // Producer
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix ("/icarus/bird1/vostping");
  producerHelper.SetAttribute ("PayloadSize", StringValue ("1024"));
  producerHelper.Install (bird); // Satellite node

  AsciiTraceHelper ascii;
  auto stream = ascii.CreateFileStream ("/tmp/out.tr");
  stream->GetStream ()->precision (9);
  icarusHelper.EnableAsciiAll (stream);
  icarusHelper.EnablePcapAll ("/tmp/pcap-sputping");

  // Add channel drops to the ASCII trace
  Config::Connect ("/NodeList/0/DeviceList/0/$ns3::icarus::IcarusNetDevice/Channel/PhyTxDrop",
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
