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
 * Author: Miguel Rodríguez Pérez <miguel@det.uvigo.gal>
 */

#include "icarus-helper.h"
#include "model/ndn-net-device-transport.hpp"
#include "ns3/abort.h"
#include "ns3/callback.h"
#include "ns3/circular-orbit.h"
#include "ns3/icarus-net-device.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/ground-sat-channel.h"
#include "ns3/log.h"
#include "ns3/mac-model.h"
#include "ns3/mobility-model.h"
#include "ns3/names.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/pointer.h"
#include "ns3/ptr.h"
#include "ns3/ground-sat-channel.h"
#include "ns3/config.h"
#include "ns3/assert.h"
#include "ns3/ground-sat-success-model.h"
#include "ns3/sat-address.h"
#include "ns3/ground-sta-transport.h"
#include "ns3/ndnSIM/NFD/daemon/face/generic-link-service.hpp"
#include "ns3/ground-node-sat-tracker.h"
#include "ns3/sat2ground-net-device.h"
#include "ns3/sat2ground-transport.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include <memory>

namespace ns3 {
namespace icarus {

NS_LOG_COMPONENT_DEFINE ("icarus.IcarusHelper");

IcarusHelper::IcarusHelper () : m_enableGeoTags (nullptr)
{
  NS_LOG_FUNCTION (this);

  m_queueFactory.SetTypeId ("ns3::DropTailQueue<Packet>");
  m_channelFactory.SetTypeId ("ns3::icarus::GroundSatChannel");
  m_sat2GroundFactory.SetTypeId ("ns3::icarus::Sat2GroundNetDevice");
  m_groundStaFactory.SetTypeId ("ns3::icarus::GroundStaNetDevice");
  m_successModelFactory.SetTypeId ("ns3::icarus::GroundSatSuccessElevation");
  m_macModelFactory.SetTypeId ("ns3::icarus::NoneMacModel");
  m_trackerModelFactory.SetTypeId ("ns3::icarus::GroundNodeSatTracker");
  m_propDelayModelFactory.SetTypeId ("ns3::ConstantSpeedPropagationDelayModel");
  m_propLossModelFactory.SetTypeId ("ns3::FriisPropagationLossModel");
}

void
IcarusHelper::SetQueue (std::string type, const std::string &n1, const AttributeValue &v1,
                        const std::string &n2, const AttributeValue &v2, const std::string &n3,
                        const AttributeValue &v3, const std::string &n4, const AttributeValue &v4)
{
  NS_LOG_FUNCTION (this << type << n1 << n2 << n3 << n4);

  QueueBase::AppendItemTypeIfNotPresent (type, "Packet");

  m_queueFactory.SetTypeId (type);
  m_queueFactory.Set (n1, v1);
  m_queueFactory.Set (n2, v2);
  m_queueFactory.Set (n3, v3);
  m_queueFactory.Set (n4, v4);
}

void
IcarusHelper::SetSuccessModel (std::string type, const std::string &n1, const AttributeValue &v1,
                               const std::string &n2, const AttributeValue &v2,
                               const std::string &n3, const AttributeValue &v3,
                               const std::string &n4, const AttributeValue &v4)
{
  NS_LOG_FUNCTION (this << type << n1 << n2 << n3 << n4);

  m_successModelFactory.SetTypeId (type);
  m_successModelFactory.Set (n1, v1);
  m_successModelFactory.Set (n2, v2);
  m_successModelFactory.Set (n3, v3);
  m_successModelFactory.Set (n4, v4);
}

void
IcarusHelper::SetPropagationDelayModel (std::string type, const std::string &n1,
                                        const AttributeValue &v1, const std::string &n2,
                                        const AttributeValue &v2, const std::string &n3,
                                        const AttributeValue &v3, const std::string &n4,
                                        const AttributeValue &v4)
{
  NS_LOG_FUNCTION (this << type << n1 << n2 << n3 << n4);

  m_propDelayModelFactory.SetTypeId (type);
  m_propDelayModelFactory.Set (n1, v1);
  m_propDelayModelFactory.Set (n2, v2);
  m_propDelayModelFactory.Set (n3, v3);
  m_propDelayModelFactory.Set (n4, v4);
}

void
IcarusHelper::SetPropagationLossModel (std::string type, const std::string &n1,
                                       const AttributeValue &v1, const std::string &n2,
                                       const AttributeValue &v2, const std::string &n3,
                                       const AttributeValue &v3, const std::string &n4,
                                       const AttributeValue &v4)
{
  NS_LOG_FUNCTION (this << type << n1 << n2 << n3 << n4);

  m_propLossModelFactory.SetTypeId (type);
  m_propLossModelFactory.Set (n1, v1);
  m_propLossModelFactory.Set (n2, v2);
  m_propLossModelFactory.Set (n3, v3);
  m_propLossModelFactory.Set (n4, v4);
}

void
IcarusHelper::SetMacModel (std::string type, const std::string &n1, const AttributeValue &v1,
                           const std::string &n2, const AttributeValue &v2, const std::string &n3,
                           const AttributeValue &v3, const std::string &n4,
                           const AttributeValue &v4)
{
  NS_LOG_FUNCTION (this << type << n1 << n2 << n3 << n4);

  m_macModelFactory.SetTypeId (type);
  m_macModelFactory.Set (n1, v1);
  m_macModelFactory.Set (n2, v2);
  m_macModelFactory.Set (n3, v3);
  m_macModelFactory.Set (n4, v4);
}

void
IcarusHelper::SetTrackerModel (std::string type, const std::string &n1, const AttributeValue &v1,
                               const std::string &n2, const AttributeValue &v2,
                               const std::string &n3, const AttributeValue &v3,
                               const std::string &n4, const AttributeValue &v4)
{
  NS_LOG_FUNCTION (this << type << n1 << n2 << n3 << n4);

  m_trackerModelFactory.SetTypeId (type);
  m_trackerModelFactory.Set (n1, v1);
  m_trackerModelFactory.Set (n2, v2);
  m_trackerModelFactory.Set (n3, v3);
  m_trackerModelFactory.Set (n4, v4);
}

void
IcarusHelper::SetDeviceAttribute (const std::string &n1, const AttributeValue &v1)
{
  NS_LOG_FUNCTION (this << n1);

  m_sat2GroundFactory.Set (n1, v1);
  m_groundStaFactory.Set (n1, v1);
}

void
IcarusHelper::SetChannelAttribute (const std::string &n1, const AttributeValue &v1)
{
  NS_LOG_FUNCTION (this << n1);

  m_channelFactory.Set (n1, v1);
}

NetDeviceContainer
IcarusHelper::Install (Ptr<Node> node, Ptr<GroundSatChannel> channel,
                       ConstellationHelper &chelper) const
{
  NS_LOG_FUNCTION (this << node << channel << &chelper);

  return NetDeviceContainer (InstallPriv (node, channel, chelper));
}

NetDeviceContainer
IcarusHelper::Install (Ptr<Node> node, const std::string &channelName,
                       ConstellationHelper &chelper) const
{
  NS_LOG_FUNCTION (this << node << channelName << &chelper);

  Ptr<GroundSatChannel> channel = Names::Find<GroundSatChannel> (channelName);
  return NetDeviceContainer (InstallPriv (node, channel, chelper));
}

NetDeviceContainer
IcarusHelper::Install (const std::string &nodeName, Ptr<GroundSatChannel> channel,
                       ConstellationHelper &chelper) const
{
  NS_LOG_FUNCTION (this << nodeName << channel << &chelper);

  Ptr<Node> node = Names::Find<Node> (nodeName);
  return NetDeviceContainer (InstallPriv (node, channel, chelper));
}

NetDeviceContainer
IcarusHelper::Install (const std::string &nodeName, const std::string &channelName,
                       ConstellationHelper &chelper) const
{
  NS_LOG_FUNCTION (this << nodeName << channelName << &chelper);

  Ptr<Node> node = Names::Find<Node> (nodeName);
  Ptr<GroundSatChannel> channel = Names::Find<GroundSatChannel> (channelName);
  return NetDeviceContainer (InstallPriv (node, channel, chelper));
}

NetDeviceContainer
IcarusHelper::Install (const NodeContainer &c, ConstellationHelper &chelper) const
{
  NS_LOG_FUNCTION (this << &c << &chelper);

  Ptr<GroundSatChannel> channel = m_channelFactory.Create ()->GetObject<GroundSatChannel> ();
  channel->SetAttribute (
      "TxSuccess",
      PointerValue (m_successModelFactory.Create ()->GetObject<GroundSatSuccessModel> ()));
  channel->SetAttribute (
      "PropDelayModel",
      PointerValue (m_propDelayModelFactory.Create ()->GetObject<PropagationDelayModel> ()));
  channel->SetAttribute (
      "PropLossModel",
      PointerValue (m_propLossModelFactory.Create ()->GetObject<PropagationLossModel> ()));

  return Install (c, channel, chelper);
}

NetDeviceContainer
IcarusHelper::Install (const NodeContainer &c, Ptr<GroundSatChannel> channel,
                       ConstellationHelper &chelper) const
{
  NS_LOG_FUNCTION (this << &c << channel << &chelper);

  NetDeviceContainer devices;

  channel->SetConstellation (chelper.GetConstellation ());

  for (Ptr<Node> node : c)
    {
      devices.Add (InstallPriv (node, channel, chelper));
    }

  return devices;
}

NetDeviceContainer
IcarusHelper::Install (const NodeContainer &c, const std::string &channelName,
                       ConstellationHelper &chelper) const
{
  NS_LOG_FUNCTION (this << &c << channelName << &chelper);

  Ptr<GroundSatChannel> channel = Names::Find<GroundSatChannel> (channelName);
  return Install (c, channel, chelper);
}

Ptr<NetDevice>
IcarusHelper::InstallPriv (Ptr<Node> node, Ptr<GroundSatChannel> channel,
                           ConstellationHelper &chelper) const
{
  NS_LOG_FUNCTION (this << node << channel << &chelper);

  Ptr<IcarusNetDevice> device = CreateDeviceForNode (node, chelper);
  auto queue = m_queueFactory.Create<Queue<Packet>> ();
  device->SetQueue (queue);
  device->Attach (channel);
  // Aggregate a NetDeviceQueueInterface object
  Ptr<NetDeviceQueueInterface> ndqi = CreateObject<NetDeviceQueueInterface> ();
  ndqi->GetTxQueue (0)->ConnectQueueTraces (queue);
  device->AggregateObject (ndqi);

  return device;
}

Ptr<IcarusNetDevice>
IcarusHelper::CreateDeviceForNode (Ptr<Node> node, ConstellationHelper &chelper) const
{
  NS_LOG_FUNCTION (this << node << &chelper);

  // Install an Orbit it if does not have already a MobilityModel
  if (node->GetObject<MobilityModel> () == nullptr)
    {
      auto sat_device = m_sat2GroundFactory.Create<Sat2GroundNetDevice> ();
      sat_device->SetAttribute ("MacModelRx", PointerValue (m_macModelFactory.Create<MacModel> ()));
      node->AddDevice (sat_device);
      const auto address = chelper.LaunchSatellite (sat_device);
      sat_device->SetAddress (address.ConvertTo ());

      return sat_device;
    }

  // This is NOT a satellite
  const auto ground_device = m_groundStaFactory.Create<GroundStaNetDevice> ();
  ground_device->SetAttribute ("MacModelTx", PointerValue (m_macModelFactory.Create<MacModel> ()));
  ground_device->SetAddress (Mac48Address::Allocate ());
  node->AddDevice (ground_device);

  auto tracker = m_trackerModelFactory.Create<GroundNodeSatTracker> ();
  node->AggregateObject (tracker);
  tracker->Start ();

  return ground_device;
}

void
IcarusHelper::EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous,
                                  bool explicitFilename)
{
  //
  // All of the Pcap enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type IcarusHelper.
  //
  Ptr<IcarusNetDevice> device = nd->GetObject<GroundStaNetDevice> ();
  if (device == nullptr)
    {
      // Try the satellite device
      device = nd->GetObject<Sat2GroundNetDevice> ();
    }
  if (device == nullptr)
    {
      NS_LOG_INFO ("IcarusHelper::EnablePcapInternal(): Device "
                   << device << " not of type ns3::icarus::IcarusNetDevice");
      return;
    }

  PcapHelper pcapHelper;

  std::string filename;
  if (explicitFilename)
    {
      filename = prefix;
    }
  else
    {
      filename = pcapHelper.GetFilenameFromDevice (prefix, device);
    }

  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (filename, std::ios::out, PcapHelper::DLT_RAW);
  if (promiscuous)
    {
      pcapHelper.HookDefaultSink<IcarusNetDevice> (device, "PromiscSniffer", file);
    }
  else
    {
      pcapHelper.HookDefaultSink<IcarusNetDevice> (device, "Sniffer", file);
    }
}

void
IcarusHelper::EnableAsciiInternal (Ptr<OutputStreamWrapper> stream, std::string prefix,
                                   Ptr<NetDevice> nd, bool explicitFilename)
{
  //
  // All of the ascii enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type IcarusNetDevice.
  //
  Ptr<IcarusNetDevice> device = nd->GetObject<IcarusNetDevice> ();
  if (device == nullptr)
    {
      NS_LOG_INFO ("IcarusHelper::EnableAsciiInternal(): Device "
                   << device << " not of type ns3::icarus::IcarusNetDevice");
      return;
    }

  //
  // Our default trace sinks are going to use packet printing, so we have to
  // make sure that is turned on.
  //
  Packet::EnablePrinting ();

  //
  // If we are not provided an OutputStreamWrapper, we are expected to create
  // one using the usual trace filename conventions and do a Hook*WithoutContext
  // since there will be one file per context and therefore the context would
  // be redundant.
  //
  if (stream == nullptr)
    {
      //
      // Set up an output stream object to deal with private ofstream copy
      // constructor and lifetime issues.  Let the helper decide the actual
      // name of the file given the prefix.
      //
      AsciiTraceHelper asciiTraceHelper;

      std::string filename;
      if (explicitFilename)
        {
          filename = prefix;
        }
      else
        {
          filename = asciiTraceHelper.GetFilenameFromDevice (prefix, device);
        }

      Ptr<OutputStreamWrapper> theStream = asciiTraceHelper.CreateFileStream (filename);

      //
      // The MacRx trace source provides our "r" event.
      //
      asciiTraceHelper.HookDefaultReceiveSinkWithoutContext<IcarusNetDevice> (device, "MacRx",
                                                                              theStream);

      //
      // The "+", '-', and 'd' events are driven by trace sources actually in the
      // transmit queue.
      //
      Ptr<Queue<Packet>> queue = device->GetQueue ();
      asciiTraceHelper.HookDefaultEnqueueSinkWithoutContext<Queue<Packet>> (queue, "Enqueue",
                                                                            theStream);
      asciiTraceHelper.HookDefaultDropSinkWithoutContext<Queue<Packet>> (queue, "Drop", theStream);
      asciiTraceHelper.HookDefaultDequeueSinkWithoutContext<Queue<Packet>> (queue, "Dequeue",
                                                                            theStream);

      return;
    }

  //
  // If we are provided an OutputStreamWrapper, we are expected to use it, and
  // to provide a context.  We are free to come up with our own context if we
  // want, and use the AsciiTraceHelper Hook*WithContext functions, but for
  // compatibility and simplicity, we just use Config::Connect and let it deal
  // with the context.
  //
  // Note that we are going to use the default trace sinks provided by the
  // ascii trace helper.  There is actually no AsciiTraceHelper in sight here,
  // but the default trace sinks are actually publicly available static
  // functions that are always there waiting for just such a case.
  //
  uint32_t nodeid = nd->GetNode ()->GetId ();
  uint32_t deviceid = nd->GetIfIndex ();

  std::string name{"icarus::Sat2GroundNetDevice"};
  if (DynamicCast<GroundStaNetDevice> (device) != nullptr)
    {
      name = "icarus::GroundStaNetDevice";
    }
  else
    {
      NS_ASSERT (DynamicCast<Sat2GroundNetDevice> (device) != nullptr);
    }

  std::ostringstream oss;

  oss << "/NodeList/" << nd->GetNode ()->GetId () << "/DeviceList/" << deviceid << "/$ns3::" << name
      << "/MacRx";
  Config::Connect (oss.str (),
                   MakeBoundCallback (&AsciiTraceHelper::DefaultReceiveSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::" << name
      << "/TxQueue/Enqueue";
  Config::Connect (oss.str (),
                   MakeBoundCallback (&AsciiTraceHelper::DefaultEnqueueSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::" << name
      << "/TxQueue/Dequeue";
  Config::Connect (oss.str (),
                   MakeBoundCallback (&AsciiTraceHelper::DefaultDequeueSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::" << name
      << "/TxQueue/Drop";
  Config::Connect (oss.str (),
                   MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));
}

void
IcarusHelper::SetEnableGeoTags (std::function<std::shared_ptr<ndn::lp::GeoTag> ()> enableGeoTags)
{
  m_enableGeoTags = enableGeoTags;
  return;
}

// Adapted from ndn-stack-helper.cpp
std::string
IcarusHelper::constructFaceUri (Ptr<NetDevice> netDevice)
{
  std::string uri = "netdev://";
  Address address = netDevice->GetAddress ();
  if (Mac48Address::IsMatchingType (address))
    {
      uri += "[" + boost::lexical_cast<std::string> (Mac48Address::ConvertFrom (address)) + "]";
    }

  return uri;
}

std::shared_ptr<nfd::face::Face>
IcarusHelper::GroundStaNetDeviceCallback (Ptr<Node> node, Ptr<ndn::L3Protocol> ndn,
                                          Ptr<NetDevice> netDevice)
{
  NS_LOG_DEBUG ("Creating default Face on node " << node->GetId ());

  // Create an ndnSIM-specific transport instance
  ::nfd::face::GenericLinkService::Options opts;
  opts.allowFragmentation = true;
  opts.allowReassembly = true;
  opts.allowCongestionMarking = true;
  opts.enableGeoTags = m_enableGeoTags;

  auto linkService = std::make_unique<::nfd::face::GenericLinkService> (opts);

  auto transport = std::make_unique<ndn::icarus::GroundStaTransport> (
      node, netDevice, constructFaceUri (netDevice), "satdev://[0000:0000:0000]");

  auto face = std::make_shared<nfd::face::Face> (std::move (linkService), std::move (transport));
  face->setMetric (1);

  ndn->addFace (face);
  NS_LOG_LOGIC ("Node " << node->GetId () << ": added Face as face #" << face->getLocalUri ());

  return face;
}

std::shared_ptr<nfd::face::Face>
IcarusHelper::Sat2GroundNetDeviceCallback (Ptr<Node> node, Ptr<ndn::L3Protocol> ndn,
                                           Ptr<NetDevice> netDevice)
{
  NS_LOG_DEBUG ("Creating default Face on node " << node->GetId ());

  // Create an ndnSIM-specific transport instance
  ::nfd::face::GenericLinkService::Options opts;
  opts.allowFragmentation = true;
  opts.allowReassembly = true;
  opts.allowCongestionMarking = true;
  /* Enable GeoTags just to prevent GenericLinkService from discarding incoming ones */
  opts.enableGeoTags = [] () -> std::shared_ptr<ndn::lp::GeoTag> { return nullptr; };

  auto linkService = std::make_unique<::nfd::face::GenericLinkService> (opts);

  auto transport = std::make_unique<ndn::icarus::Sat2GroundTransport> (
      node, netDevice, constructFaceUri (netDevice), "netdev://[ff:ff:ff:ff:ff:ff]");

  auto face = std::make_shared<nfd::face::Face> (std::move (linkService), std::move (transport));
  face->setMetric (1);

  ndn->addFace (face);
  NS_LOG_LOGIC ("Node " << node->GetId () << ": added Face as face #" << face->getLocalUri ());

  return face;
}

void
IcarusHelper::FixNdnStackHelper (ndn::StackHelper &sh)
{
  NS_LOG_FUNCTION (&sh);

  sh.AddFaceCreateCallback (GroundStaNetDevice::GetTypeId (),
                            MakeCallback (&IcarusHelper::GroundStaNetDeviceCallback, this));
  sh.AddFaceCreateCallback (Sat2GroundNetDevice::GetTypeId (),
                            MakeCallback (&IcarusHelper::Sat2GroundNetDeviceCallback, this));
}

} // namespace icarus
} // namespace ns3
