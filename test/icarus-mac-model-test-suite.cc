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

#include "ns3/application-container.h"
#include "ns3/config.h"
#include "ns3/data-rate.h"
#include "ns3/icarus-helper.h"
#include "ns3/icarus-module.h"
#include "ns3/icarus-net-device.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-header.h"
#include "ns3/log.h"
#include "ns3/mobility-module.h"
#include "ns3/object.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/test.h"
#include "ns3/udp-header.h"
#include "src/core/model/log-macros-disabled.h"
#include "src/network/utils/packet-data-calculators.h"

#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/si/prefixes.hpp>
#include <boost/math/constants/constants.hpp>
#include <ios>

using namespace ns3;
using namespace icarus;

NS_LOG_COMPONENT_DEFINE ("ns3.icarus.MacModelTestSuite");

namespace {

std::string
GetTestName (double g) noexcept
{
  std::ostringstream name ("Slotted Aloha g=", std::ios_base::ate);
  name << g;

  return name.str ();
}

class SlottedAloha : public TestCase
{
public:
  SlottedAloha (double g);

private:
  const double m_g;
  const std::size_t m_nodes;
  const std::size_t m_payloadSize;
  const Time m_transmissionDuration;
  const DataRate m_channelDataRate;
  NodeContainer m_nodesContainer;
  ApplicationContainer m_clientApps, m_sinkApps;

  virtual void DoSetup () override;
  virtual void DoRun () override;
};

SlottedAloha::SlottedAloha (double g)
    : TestCase (GetTestName (g)),
      m_g (g),
      m_nodes (250),
      m_payloadSize (100),
      m_transmissionDuration (Seconds (1)),
      m_channelDataRate (DataRate ("100Mbps"))
{
  NS_LOG_FUNCTION (this);
}

void
SlottedAloha::DoSetup ()
{
  NS_LOG_FUNCTION (this);
  using boost::units::quantity;
  using boost::units::degree::degrees;
  using boost::units::si::kilo;
  using boost::units::si::length;
  using boost::units::si::meters;
  using boost::units::si::plane_angle;

  Config::SetDefault ("ns3::icarus::IcarusNetDevice::DataRate", DataRateValue (m_channelDataRate));
  Config::SetDefault ("ns3::icarus::GroundNodeSatTracker::TrackingInterval",
                      TimeValue (Minutes (1)));

  m_nodesContainer.Create (m_nodes);

  ConstellationHelper constelHelper (quantity<length> (250 * kilo * meters),
                                     quantity<plane_angle> (60.0 * degrees), 1, 1, 0);

  /* Setting positions and mobility model to the ground nodes */
  ObjectFactory staticPositionsFactory ("ns3::ListPositionAllocator");
  auto staticPositions = staticPositionsFactory.Create<ListPositionAllocator> ();
  staticPositions->Add (GeographicPositions::GeographicToCartesianCoordinates (
      42.1704632, -8.6877909, 450, GeographicPositions::WGS84)); // Our School
  MobilityHelper staticHelper;
  staticHelper.SetPositionAllocator (staticPositions);
  staticHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  staticHelper.Install (m_nodesContainer);

  auto sat_node = CreateObject<Node> ();
  m_nodesContainer.Add (sat_node);

  const auto header_size = Ipv4Header ().GetSerializedSize () + UdpHeader ().GetSerializedSize ();
  const auto pkt_tx_time =
      DataRate (m_channelDataRate).CalculateBytesTxTime (m_payloadSize + header_size + 1);

  IcarusHelper icarusHelper;
  icarusHelper.SetMacModel ("ns3::icarus::AlohaMacModel", "SlotDuration", TimeValue (pkt_tx_time));
  auto netDevices (icarusHelper.Install (m_nodesContainer, constelHelper));

  /* Configuring IP stack at the nodes */
  Config::SetDefault ("ns3::Ipv4::IpForward", BooleanValue (false));
  InternetStackHelper ipStack;
  ipStack.Install (m_nodesContainer);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer ipInterfaces = address.Assign (netDevices);
  /* Configuring Poisson clients at the ground nodes */
  PoissonHelper clientHelper ("ns3::UdpSocketFactory",
                              Address (InetSocketAddress (ipInterfaces.GetAddress (m_nodes), 7667)),
                              DataRate (m_channelDataRate.GetBitRate () * m_g / m_nodes),
                              header_size, m_payloadSize);

  // Do not install app into satellite
  for (auto i = 0u; i < m_nodes; i++)
    {
      m_clientApps.Add (clientHelper.Install (m_nodesContainer.Get (i)));
    }

  /* Configuring traffic sink at the satellite node */
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory",
                               Address (InetSocketAddress (Ipv4Address::GetAny (), 7667)));
  m_sinkApps = sinkHelper.Install (sat_node);
}

void
SlottedAloha::DoRun ()
{
  NS_LOG_FUNCTION (this);

  auto totalRx = Create<PacketCounterCalculator> ();
  totalRx->SetKey ("rx-frames");
  Config::Connect ("/NodeList/*/DeviceList/0/$ns3::icarus::Sat2GroundNetDevice/MacRx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate, totalRx));

  auto totalTx = Create<PacketCounterCalculator> ();
  totalTx->SetKey ("tx-frames");
  Config::Connect ("/NodeList/*/DeviceList/0/$ns3::icarus::GroundStaNetDevice/TxQueue/Enqueue",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate, totalTx));

  const Time init_application_time = Seconds (268896.0);
  m_clientApps.Start (init_application_time);
  m_sinkApps.Start (init_application_time);

  m_clientApps.Stop (init_application_time + m_transmissionDuration);
  Simulator::Stop (init_application_time + m_transmissionDuration +
                   Seconds (1)); // Add 1 second to let all the packets enough time to arrive
  Simulator::Run ();

  NS_TEST_ASSERT_MSG_EQ_TOL (
      m_g * (totalRx->GetCount () / static_cast<double> (totalTx->GetCount ())), m_g * exp (-m_g),
      1e-2, "Not equal");

  Simulator::Destroy ();
}

class RegularAloha : public TestCase
{
public:
  RegularAloha ();

private:
  const double m_g;
  const std::size_t m_nodes;
  const std::size_t m_payloadSize;
  const Time m_transmissionDuration;
  const DataRate m_channelDataRate;
  NodeContainer m_nodesContainer;
  ApplicationContainer m_clientApps, m_sinkApps;

  virtual void DoSetup () override;
  virtual void DoRun () override;
};

RegularAloha::RegularAloha ()
    : TestCase ("Regular Aloha g=0.5"),
      m_g (0.5),
      m_nodes (250),
      m_payloadSize (100),
      m_transmissionDuration (Seconds (1)),
      m_channelDataRate (DataRate ("100Mbps"))
{
  NS_LOG_FUNCTION (this);
}

void
RegularAloha::DoSetup ()
{
  NS_LOG_FUNCTION (this);
  using boost::units::quantity;
  using boost::units::degree::degrees;
  using boost::units::si::kilo;
  using boost::units::si::length;
  using boost::units::si::meters;
  using boost::units::si::plane_angle;

  Config::SetDefault ("ns3::icarus::IcarusNetDevice::DataRate", DataRateValue (m_channelDataRate));
  Config::SetDefault ("ns3::icarus::GroundNodeSatTracker::TrackingInterval",
                      TimeValue (Minutes (1)));

  m_nodesContainer.Create (m_nodes);

  ConstellationHelper constelHelper (quantity<length> (250 * kilo * meters),
                                     quantity<plane_angle> (60.0 * degrees), 1, 1, 0);

  /* Setting positions and mobility model to the ground nodes */
  ObjectFactory staticPositionsFactory ("ns3::ListPositionAllocator");
  auto staticPositions = staticPositionsFactory.Create<ListPositionAllocator> ();
  staticPositions->Add (GeographicPositions::GeographicToCartesianCoordinates (
      42.1704632, -8.6877909, 450, GeographicPositions::WGS84)); // Our School
  MobilityHelper staticHelper;
  staticHelper.SetPositionAllocator (staticPositions);
  staticHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  staticHelper.Install (m_nodesContainer);

  auto sat_node = CreateObject<Node> ();
  m_nodesContainer.Add (sat_node);

  const auto header_size = Ipv4Header ().GetSerializedSize () + UdpHeader ().GetSerializedSize ();

  IcarusHelper icarusHelper;
  icarusHelper.SetMacModel ("ns3::icarus::AlohaMacModel", "SlotDuration", TimeValue (Seconds (0)));
  auto netDevices (icarusHelper.Install (m_nodesContainer, constelHelper));

  /* Configuring IP stack at the nodes */
  Config::SetDefault ("ns3::Ipv4::IpForward", BooleanValue (false));
  InternetStackHelper ipStack;
  ipStack.Install (m_nodesContainer);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer ipInterfaces = address.Assign (netDevices);
  /* Configuring Poisson clients at the ground nodes */
  PoissonHelper clientHelper ("ns3::UdpSocketFactory",
                              Address (InetSocketAddress (ipInterfaces.GetAddress (m_nodes), 7667)),
                              DataRate (m_channelDataRate.GetBitRate () * m_g / m_nodes),
                              header_size, m_payloadSize);

  // Do not install app into satellite
  for (auto i = 0u; i < m_nodes; i++)
    {
      m_clientApps.Add (clientHelper.Install (m_nodesContainer.Get (i)));
    }

  /* Configuring traffic sink at the satellite node */
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory",
                               Address (InetSocketAddress (Ipv4Address::GetAny (), 7667)));
  m_sinkApps = sinkHelper.Install (sat_node);
}

void
RegularAloha::DoRun ()
{
  NS_LOG_FUNCTION (this);
  using namespace boost::math::double_constants;

  auto totalRx = Create<PacketCounterCalculator> ();
  totalRx->SetKey ("rx-frames");
  Config::Connect ("/NodeList/*/DeviceList/0/$ns3::icarus::Sat2GroundNetDevice/MacRx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate, totalRx));

  auto totalTx = Create<PacketCounterCalculator> ();
  totalTx->SetKey ("tx-frames");
  Config::Connect ("/NodeList/*/DeviceList/0/$ns3::icarus::GroundStaNetDevice/TxQueue/Enqueue",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate, totalTx));

  const Time init_application_time = Seconds (268896.0);
  m_clientApps.Start (init_application_time);
  m_sinkApps.Start (init_application_time);

  m_clientApps.Stop (init_application_time + m_transmissionDuration);
  Simulator::Stop (init_application_time + m_transmissionDuration +
                   Seconds (1)); // Add 1 second to let all the packets enough time to arrive
  Simulator::Run ();

  NS_TEST_ASSERT_MSG_EQ_TOL (
      m_g * (totalRx->GetCount () / static_cast<double> (totalTx->GetCount ())), 1 / (2 * e), 1e-2,
      "Not equal");

  Simulator::Destroy ();
}

class CrdsaAloha : public TestCase
{
public:
  CrdsaAloha ();

private:
  const double m_g;
  const std::size_t m_nodes;
  const std::size_t m_payloadSize;
  const uint16_t m_slotsPerFrame;
  const uint16_t m_replicasPerPacket;
  const Time m_transmissionDuration;
  const DataRate m_channelDataRate;
  NodeContainer m_nodesContainer;
  ApplicationContainer m_clientApps, m_sinkApps;

  virtual void DoSetup () override;
  virtual void DoRun () override;
};

CrdsaAloha::CrdsaAloha ()
    : TestCase ("CRDSA Aloha g=0.75"),
      m_g (0.75),
      m_nodes (250),
      m_payloadSize (200),
      m_slotsPerFrame (100),
      m_replicasPerPacket (2),
      m_transmissionDuration (Seconds (1)),
      m_channelDataRate (DataRate ("100Mbps"))
{
  NS_LOG_FUNCTION (this);
}

void
CrdsaAloha::DoSetup ()
{
  NS_LOG_FUNCTION (this);
  using boost::units::quantity;
  using boost::units::degree::degrees;
  using boost::units::si::kilo;
  using boost::units::si::length;
  using boost::units::si::meters;
  using boost::units::si::plane_angle;

  Config::SetDefault ("ns3::icarus::IcarusNetDevice::DataRate", DataRateValue (m_channelDataRate));
  Config::SetDefault ("ns3::icarus::GroundNodeSatTracker::TrackingInterval",
                      TimeValue (Minutes (1)));

  m_nodesContainer.Create (m_nodes);

  ConstellationHelper constelHelper (quantity<length> (250 * kilo * meters),
                                     quantity<plane_angle> (60.0 * degrees), 1, 1, 0);

  /* Setting positions and mobility model to the ground nodes */
  ObjectFactory staticPositionsFactory ("ns3::ListPositionAllocator");
  auto staticPositions = staticPositionsFactory.Create<ListPositionAllocator> ();
  staticPositions->Add (GeographicPositions::GeographicToCartesianCoordinates (
      42.1704632, -8.6877909, 450, GeographicPositions::WGS84)); // Our School
  MobilityHelper staticHelper;
  staticHelper.SetPositionAllocator (staticPositions);
  staticHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  staticHelper.Install (m_nodesContainer);

  auto sat_node = CreateObject<Node> ();
  m_nodesContainer.Add (sat_node);

  const auto header_size = Ipv4Header ().GetSerializedSize () + UdpHeader ().GetSerializedSize ();
  const auto pkt_tx_time =
      DataRate (m_channelDataRate).CalculateBytesTxTime (m_payloadSize + header_size + 1);

  IcarusHelper icarusHelper;
  icarusHelper.SetMacModel ("ns3::icarus::CrdsaMacModel", "SlotDuration", TimeValue (pkt_tx_time),
                            "SlotsPerFrame", UintegerValue (m_slotsPerFrame), "ReplicasPerPacket",
                            UintegerValue (m_replicasPerPacket));
  auto netDevices (icarusHelper.Install (m_nodesContainer, constelHelper));

  /* Configuring IP stack at the nodes */
  Config::SetDefault ("ns3::Ipv4::IpForward", BooleanValue (false));
  InternetStackHelper ipStack;
  ipStack.Install (m_nodesContainer);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer ipInterfaces = address.Assign (netDevices);
  /* Configuring Poisson clients at the ground nodes */
  PoissonHelper clientHelper ("ns3::UdpSocketFactory",
                              Address (InetSocketAddress (ipInterfaces.GetAddress (m_nodes), 7667)),
                              DataRate (m_channelDataRate.GetBitRate () * m_g / m_nodes),
                              header_size, m_payloadSize);

  // Do not install app into satellite
  for (auto i = 0u; i < m_nodes; i++)
    {
      m_clientApps.Add (clientHelper.Install (m_nodesContainer.Get (i)));
    }

  /* Configuring traffic sink at the satellite node */
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory",
                               Address (InetSocketAddress (Ipv4Address::GetAny (), 7667)));
  m_sinkApps = sinkHelper.Install (sat_node);
}

void
CrdsaAloha::DoRun ()
{
  NS_LOG_FUNCTION (this);
  using namespace boost::math::double_constants;

  auto totalRx = Create<PacketCounterCalculator> ();
  totalRx->SetKey ("rx-frames");
  Config::Connect ("/NodeList/*/DeviceList/0/$ns3::icarus::Sat2GroundNetDevice/MacRx",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate, totalRx));

  auto totalTx = Create<PacketCounterCalculator> ();
  totalTx->SetKey ("tx-frames");
  Config::Connect ("/NodeList/*/DeviceList/0/$ns3::icarus::GroundStaNetDevice/TxQueue/Enqueue",
                   MakeCallback (&PacketCounterCalculator::PacketUpdate, totalTx));

  const Time init_application_time = Seconds (268896.0);
  m_clientApps.Start (init_application_time);
  m_sinkApps.Start (init_application_time);

  m_clientApps.Stop (init_application_time + m_transmissionDuration);
  Simulator::Stop (init_application_time + m_transmissionDuration +
                   Seconds (1)); // Add 1 second to let all the packets enough time to arrive
  Simulator::Run ();

  NS_TEST_ASSERT_MSG_EQ_TOL (
      m_g * (totalRx->GetCount () / static_cast<double> (totalTx->GetCount ())), 0.5018, 1e-2,
      "Not equal");

  Simulator::Destroy ();
}

class IcarusMacModelTestSuite : public TestSuite
{
public:
  IcarusMacModelTestSuite ();
};

IcarusMacModelTestSuite::IcarusMacModelTestSuite () : TestSuite ("icarus.mac-model", UNIT)
{
  AddTestCase (new RegularAloha, TestCase::EXTENSIVE);
  for (auto g = 0.1; g < 1; g += 0.2)
    {
      AddTestCase (new SlottedAloha (g), TestCase::EXTENSIVE);
    }
  AddTestCase (new CrdsaAloha, TestCase::EXTENSIVE);
}

// Do not forget to allocate an instance of this TestSuite
static IcarusMacModelTestSuite icarusMacModelTestSuite;
} // namespace
