/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021–2022 Universidade de Vigo
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

#include "helper/ndn-app-helper.hpp"
#include "ndn-cxx/lp/geo-tag.hpp"
#include "ns3/assert.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/ground-sat-channel.h"
#include "ns3/ground-sta-net-device.h"
#include "ns3/icarus-module.h"
#include "ns3/log-macros-disabled.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/mobility-module.h"
#include "ns3/node-container.h"
#include "ns3/node-list.h"
#include "ns3/ptr.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/sat-address.h"
#include "ns3/simulator.h"
#include "ns3/geographic-positions.h"
#include "ns3/isl-helper.h"
#include "ns3/string.h"
#include "src/ndnSIM/utils/tracers/ndn-l3-rate-tracer.hpp"
#include "table/name-tree-hashtable.hpp"

#include <boost/units/io.hpp>
#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/si/prefixes.hpp>
#include <boost/units/systems/si/length.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>

namespace ns3 {
using namespace icarus;

NS_LOG_COMPONENT_DEFINE ("icarus.Ground2GroundPingExample");

namespace {
auto
AddGeoTag () -> auto
{
  NS_LOG_FUNCTION_NOARGS ();
  auto ground_node = NodeList::GetNode (NodeList::GetNNodes () - 2);

  auto netDevice = ground_node->GetDevice (0)->GetObject<GroundStaNetDevice> ();
  NS_ASSERT (netDevice != nullptr);
  auto remote_address = SatAddress::ConvertFrom (netDevice->GetRemoteAddress ());
  auto coid = double (remote_address.getConstellationId ());
  auto plane = double (remote_address.getOrbitalPlane ());
  auto pindex = double (remote_address.getPlaneIndex ());

  return std::make_shared<ndn::lp::GeoTag> (
      std::make_tuple (double (coid), double (plane), double (pindex)));
}
} // namespace */

auto
main (int argc, char **argv) -> int
{
  using namespace boost::units;
  using namespace boost::units::si;

  // Track best satellite every minute
  Config::SetDefault ("ns3::icarus::GroundNodeSatTracker::TrackingInterval",
                      TimeValue (Seconds (1)));
  std::string fileTrace = "/tmp/rate-trace.txt";
  double lat = 42;
  int altitude = 400;

  CommandLine cmd;
  cmd.AddValue ("trackingInterval", "ns3::icarus::GroundNodeSatTracker::TrackingInterval");
  cmd.AddValue ("fileTrace", "File for the trace", fileTrace);
  cmd.AddValue ("latitude", "Latitude of the Consumer", lat);
  cmd.AddValue ("altitude", "Altitude of the constellation", altitude);
  cmd.Parse (argc, argv);

  auto n_planes = 30;
  auto plane_size = 30;
  Ptr<UniformRandomVariable> m_uniformRandomVariable = CreateObject<UniformRandomVariable> ();
  auto random_var = m_uniformRandomVariable->GetValue (-0.1, 0.1);

  uint32_t n_nodes = (n_planes * plane_size) + 2;
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
  icarusHelper.SetSuccessModel ("ns3::icarus::GroundSatSuccessElevation", "MinElevation",
                                DoubleValue (25.0));
  icarusHelper.SetEnableGeoTags (AddGeoTag);
  ISLHelper islHelper;
  ConstellationHelper constellationHelper (quantity<length> (altitude * kilo * meters),
                                           quantity<plane_angle> (60 * degree::degree), n_planes,
                                           plane_size, 1);

  ObjectFactory staticPositionsFactory ("ns3::ListPositionAllocator");
  auto staticPositions = staticPositionsFactory.Create<ListPositionAllocator> ();
  /* staticPositions->Add (GeographicPositions::GeographicToCartesianCoordinates (
      0.5709895206068359, 25.204342819331025, 447, GeographicPositions::WGS84)); // Kisangani, Congo

  staticPositions->Add (GeographicPositions::GeographicToCartesianCoordinates (
      -0.17904073031620213, -78.47517894655105, 2850,
      GeographicPositions::WGS84)); // Quito, Ecuador */

  /* staticPositions->Add (GeographicPositions::GeographicToCartesianCoordinates (
      42.1704632, -8.6877909, 450, GeographicPositions::WGS84)); // Our School */
  staticPositions->Add (GeographicPositions::GeographicToCartesianCoordinates (
      lat, -8.6877909 + random_var, 450, GeographicPositions::WGS84));
  staticPositions->Add (GeographicPositions::GeographicToCartesianCoordinates (
      lat, -74.013382, -17, GeographicPositions::WGS84)); // World Trade Center, NYC
  MobilityHelper staticHelper;
  staticHelper.SetPositionAllocator (staticPositions);
  staticHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  staticHelper.Install (ground1);
  staticHelper.Install (ground2);
  icarusHelper.Install (nodes, constellationHelper);

  islHelper.Install (birds, constellationHelper);

  // Install NDN stack on all nodes
  ns3::ndn::StackHelper ndnHelper;
  icarusHelper.FixNdnStackHelper (ndnHelper);
  islHelper.FixNdnStackHelper (ndnHelper);
  ndnHelper.SetDefaultRoutes (true);
  ndnHelper.InstallAll ();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll ("/icarus", "/localhost/nfd/strategy/geo-tag");

  // Installing applications

  // Consumer
  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix ("/icarus/ground2/vostping");
  consumerHelper.SetAttribute ("Frequency", StringValue ("10"));
  auto apps = consumerHelper.Install (ground1);

  // Producer
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix ("/icarus/ground2/vostping");
  producerHelper.SetAttribute ("PayloadSize", StringValue ("1024"));
  producerHelper.Install (ground2); // Satellite node

  //AsciiTraceHelper ascii;
  //auto stream = ascii.CreateFileStream ("/home/pablo/tmp/out.tr");
  //stream->GetStream ()->precision (9);
  //icarusHelper.EnableAsciiAll (stream);
  //icarusHelper.EnablePcapAll ("/home/pablo/tmp/pcap-sputping");

  // Add channel drops to the ASCII trace
  //Config::Connect ("/NodeList/0/DeviceList/0/$ns3::icarus::IcarusNetDevice/Channel/PhyTxDrop",
  //                 MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));

  ns3::Simulator::Stop (Seconds (3601));

  ndn::L3RateTracer::Install (ground1, fileTrace, Seconds (60));

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