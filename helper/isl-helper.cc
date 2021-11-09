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

#include "isl-helper.h"
#include "ns3/abort.h"
#include "ns3/circular-orbit.h"
#include "ns3/sat-net-device.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/sat2sat-channel.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/names.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/pointer.h"
#include "ns3/ptr.h"
#include "ns3/config.h"
#include "ns3/assert.h"
#include "ns3/sat2sat-success-model.h"
#include "src/icarus/utils/sat-address.h"

namespace ns3 {
namespace icarus {

NS_LOG_COMPONENT_DEFINE ("icarus.ISLHelper");

ISLHelper::ISLHelper ()
{
  NS_LOG_FUNCTION (this);

  m_queueFactory.SetTypeId ("ns3::DropTailQueue<Packet>");
  m_channelFactory.SetTypeId ("ns3::icarus::Sat2SatChannel");
  m_satNetDeviceFactory.SetTypeId ("ns3::icarus::SatNetDevice");
  m_successModelFactory.SetTypeId ("ns3::icarus::Sat2SatSuccessModel");
}

void
ISLHelper::SetQueue (std::string type, const std::string &n1, const AttributeValue &v1,
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
ISLHelper::SetSuccessModel (std::string type, const std::string &n1, const AttributeValue &v1,
                               const std::string &n2, const AttributeValue &v2,
                               const std::string &n3, const AttributeValue &v3,
                               const std::string &n4, const AttributeValue &v4)
{
  NS_LOG_FUNCTION (this << type << n1 << n2 << n3 << n4);

  QueueBase::AppendItemTypeIfNotPresent (type, "Packet");

  m_successModelFactory.SetTypeId (type);
  m_successModelFactory.Set (n1, v1);
  m_successModelFactory.Set (n2, v2);
  m_successModelFactory.Set (n3, v3);
  m_successModelFactory.Set (n4, v4);
}

void
ISLHelper::SetDeviceAttribute (const std::string &n1, const AttributeValue &v1)
{
  NS_LOG_FUNCTION (this << n1);

  m_satNetDeviceFactory.Set (n1, v1);
}

void
ISLHelper::SetChannelAttribute (const std::string &n1, const AttributeValue &v1)
{
  NS_LOG_FUNCTION (this << n1);

  m_channelFactory.Set (n1, v1);
}

NetDeviceContainer
ISLHelper::Install (Ptr<Node> node, Ptr<Sat2SatChannel> channel,
                       ConstellationHelper *chelper) const
{
  NS_LOG_FUNCTION (this << node << channel << chelper);

  return NetDeviceContainer (InstallPriv (node, channel, chelper));
}

NetDeviceContainer
ISLHelper::Install (Ptr<Node> node, const std::string &channelName,
                       ConstellationHelper *chelper) const
{
  NS_LOG_FUNCTION (this << node << channelName << chelper);

  Ptr<Sat2SatChannel> channel = Names::Find<Sat2SatChannel> (channelName);
  return NetDeviceContainer (InstallPriv (node, channel, chelper));
}

NetDeviceContainer
ISLHelper::Install (const std::string &nodeName, Ptr<Sat2SatChannel> channel,
                       ConstellationHelper *chelper) const
{
  NS_LOG_FUNCTION (this << nodeName << channel << chelper);

  Ptr<Node> node = Names::Find<Node> (nodeName);
  return NetDeviceContainer (InstallPriv (node, channel, chelper));
}

NetDeviceContainer
ISLHelper::Install (const std::string &nodeName, const std::string &channelName,
                       ConstellationHelper *chelper) const
{
  NS_LOG_FUNCTION (this << nodeName << channelName << chelper);

  Ptr<Node> node = Names::Find<Node> (nodeName);
  Ptr<Sat2SatChannel> channel = Names::Find<Sat2SatChannel> (channelName);
  return NetDeviceContainer (InstallPriv (node, channel, chelper));
}

NetDeviceContainer
ISLHelper::Install (const NodeContainer &c, ConstellationHelper *chelper) const
{
  NS_LOG_FUNCTION (this << &c << chelper);

  Ptr<Sat2SatChannel> channel = m_channelFactory.Create ()->GetObject<Sat2SatChannel> ();
  channel->SetAttribute (
      "TxSuccess",
      PointerValue (m_successModelFactory.Create ()->GetObject<Sat2SatSuccessModel> ()));

  return Install (c, channel, chelper);
}

NetDeviceContainer
ISLHelper::Install (const NodeContainer &c, Ptr<Sat2SatChannel> channel,
                       ConstellationHelper *chelper) const
{
  NS_LOG_FUNCTION (this << &c << channel << chelper);

  NetDeviceContainer devices;

  for (Ptr<Node> node : c)
    {
      devices.Add (InstallPriv (node, channel, chelper));
    }

  return devices;
}

NetDeviceContainer
ISLHelper::Install (const NodeContainer &c, const std::string &channelName,
                       ConstellationHelper *chelper) const
{
  NS_LOG_FUNCTION (this << &c << channelName << chelper);

  Ptr<Sat2SatChannel> channel = Names::Find<Sat2SatChannel> (channelName);
  return Install (c, channel, chelper);
}

Ptr<NetDevice>
ISLHelper::InstallPriv (Ptr<Node> node, Ptr<Sat2SatChannel> channel,
                           ConstellationHelper *chelper) const
{
  NS_LOG_FUNCTION (this << node << channel << chelper);

  Ptr<SatNetDevice> device = CreateDeviceForNode (node, chelper);
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

Ptr<SatNetDevice>
ISLHelper::CreateDeviceForNode (Ptr<Node> node, ConstellationHelper *chelper) const
{
  NS_LOG_FUNCTION (this << node << chelper);

  // Install an Orbit it if does not have already a MobilityModel
  if (node->GetObject<MobilityModel> () == nullptr)
    {
      NS_ASSERT_MSG (chelper != nullptr,
                     "We need a ConstellationHelper to create a Satellite device");
      auto sat_device = m_satNetDeviceFactory.Create<SatNetDevice> ();
      const auto address = chelper->LaunchSatellite (node);

      return sat_device;
    }
}

void
ISLHelper::EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous,
                                  bool explicitFilename)
{
  //
  // All of the Pcap enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type ISLHelper.
  //
  Ptr<SatNetDevice> device = nd->GetObject<SatNetDevice> ();
  if (device == 0)
    {
      // Try the satellite device
      device = nd->GetObject<SatNetDevice> ();
    }
  if (device == 0)
    {
      NS_LOG_INFO ("ISLHelper::EnablePcapInternal(): Device "
                   << device << " not of type ns3::icarus::SatNetDevice");
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
      pcapHelper.HookDefaultSink<SatNetDevice> (device, "PromiscSniffer", file);
    }
  else
    {
      pcapHelper.HookDefaultSink<SatNetDevice> (device, "Sniffer", file);
    }
}

void
ISLHelper::EnableAsciiInternal (Ptr<OutputStreamWrapper> stream, std::string prefix,
                                   Ptr<NetDevice> nd, bool explicitFilename)
{
  //
  // All of the ascii enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type SatNetDevice.
  //
  Ptr<SatNetDevice> device = nd->GetObject<SatNetDevice> ();
  if (device == 0)
    {
      NS_LOG_INFO ("ISLHelper::EnableAsciiInternal(): Device "
                   << device << " not of type ns3::icarus::SatNetDevice");
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
      asciiTraceHelper.HookDefaultReceiveSinkWithoutContext<SatNetDevice> (device, "MacRx",
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

  std::string name{"icarus::SatNetDevice"};
  if (DynamicCast<SatNetDevice> (device) != 0)
    {
      name = "icarus::SatNetDevice";
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
