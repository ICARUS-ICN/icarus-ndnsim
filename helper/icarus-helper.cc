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

#include "icarus-helper.h"
#include "ns3/abort.h"
#include "ns3/circular-orbit.h"
#include "ns3/icarus-net-device.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/ground-sat-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/names.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/ptr.h"
#include "ns3/ground-sat-channel.h"
#include "ns3/config.h"
#include "ns3/assert.h"

namespace ns3 {
namespace icarus {

NS_LOG_COMPONENT_DEFINE ("icarus.IcarusHelper");

IcarusHelper::IcarusHelper ()
{
  NS_LOG_FUNCTION (this);

  m_queueFactory.SetTypeId ("ns3::DropTailQueue<Packet>");
  m_channelFactory.SetTypeId ("ns3::icarus::GroundSatChannel");
  m_sat2GroundFactory.SetTypeId ("ns3::icarus::Sat2GroundNetDevice");
  m_groundStaFactory.SetTypeId ("ns3::icarus::GroundStaNetDevice");
}

void
IcarusHelper::SetQueue (std::string type, std::string n1, const AttributeValue &v1, std::string n2,
                        const AttributeValue &v2, std::string n3, const AttributeValue &v3,
                        std::string n4, const AttributeValue &v4)
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
IcarusHelper::SetDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  NS_LOG_FUNCTION (this << n1);

  m_sat2GroundFactory.Set (n1, v1);
  m_groundStaFactory.Set (n1, v1);
}

void
IcarusHelper::SetChannelAttribute (std::string n1, const AttributeValue &v1)
{
  NS_LOG_FUNCTION (this << n1);

  m_channelFactory.Set (n1, v1);
}

NetDeviceContainer
IcarusHelper::Install (Ptr<Node> node, Ptr<GroundSatChannel> channel) const
{
  NS_LOG_FUNCTION (this << node << channel);

  return NetDeviceContainer (InstallPriv (node, channel));
}

NetDeviceContainer
IcarusHelper::Install (Ptr<Node> node, std::string channelName) const
{
  NS_LOG_FUNCTION (this << node << channelName);

  Ptr<GroundSatChannel> channel = Names::Find<GroundSatChannel> (channelName);
  return NetDeviceContainer (InstallPriv (node, channel));
}

NetDeviceContainer
IcarusHelper::Install (std::string nodeName, Ptr<GroundSatChannel> channel) const
{
  NS_LOG_FUNCTION (this << nodeName << channel);

  Ptr<Node> node = Names::Find<Node> (nodeName);
  return NetDeviceContainer (InstallPriv (node, channel));
}

NetDeviceContainer
IcarusHelper::Install (std::string nodeName, std::string channelName) const
{
  NS_LOG_FUNCTION (this << nodeName << channelName);

  Ptr<Node> node = Names::Find<Node> (nodeName);
  Ptr<GroundSatChannel> channel = Names::Find<GroundSatChannel> (channelName);
  return NetDeviceContainer (InstallPriv (node, channel));
}

NetDeviceContainer
IcarusHelper::Install (const NodeContainer &c) const
{
  NS_LOG_FUNCTION (this << &c);

  Ptr<GroundSatChannel> channel = m_channelFactory.Create ()->GetObject<GroundSatChannel> ();

  return Install (c, channel);
}

NetDeviceContainer
IcarusHelper::Install (const NodeContainer &c, Ptr<GroundSatChannel> channel) const
{
  NS_LOG_FUNCTION (this << &c << channel);

  NetDeviceContainer devices;

  for (Ptr<Node> node : c)
    {
      devices.Add (InstallPriv (node, channel));
    }

  return devices;
}

NetDeviceContainer
IcarusHelper::Install (const NodeContainer &c, std::string channelName) const
{
  NS_LOG_FUNCTION (this << &c << channelName);

  Ptr<GroundSatChannel> channel = Names::Find<GroundSatChannel> (channelName);
  return Install (c, channel);
}

Ptr<NetDevice>
IcarusHelper::InstallPriv (Ptr<Node> node, Ptr<GroundSatChannel> channel) const
{
  NS_LOG_FUNCTION (this << node << channel);

  Ptr<IcarusNetDevice> device = CreateDeviceForNode (node);
  device->SetAddress (Mac48Address::Allocate ());
  node->AddDevice (device);
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
IcarusHelper::CreateDeviceForNode (Ptr<Node> node) const
{
  NS_LOG_FUNCTION (this << node);

  NS_ABORT_MSG_UNLESS (node->GetObject<MobilityModel> () != 0,
                       "Must assign a mobility model to the node BEFORE installing its netdevice.");
  if (node->GetObject<CircularOrbitMobilityModel> () != 0)
    {
      // This is a satellite
      return m_sat2GroundFactory.Create<Sat2GroundNetDevice> ();
    }

  // This is NOT a satellite
  return m_groundStaFactory.Create<GroundStaNetDevice> ();
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
  if (device == 0)
    {
      // Try the satellite device
      device = nd->GetObject<Sat2GroundNetDevice> ();
    }
  if (device == 0)
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
  if (device == 0)
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
  if (stream == 0)
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

  std::string name = "icarus::Sat2GroundNetDevice";
  if (DynamicCast<GroundStaNetDevice> (device) != 0)
    {
      name = "icarus::GroundStaNetDevice";
    }
  else
    {
      NS_ASSERT (DynamicCast<Sat2GroundNetDevice> (device) != 0);
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

} // namespace icarus
} // namespace ns3
