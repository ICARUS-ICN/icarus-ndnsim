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
#include "ns3/sat-address.h"

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
                            const std::string &n2, const AttributeValue &v2, const std::string &n3,
                            const AttributeValue &v3, const std::string &n4,
                            const AttributeValue &v4)
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
ISLHelper::Install (const NodeContainer &c, ConstellationHelper *chelper)
{
  NetDeviceContainer devices;
  uint16_t constellationId = 0;

  for (auto node = c.Begin (); node != c.End (); node++)
    {
      Ptr<NetDevice> netDevice = (*node)->GetDevice (0);
      Ptr<Sat2GroundNetDevice> sat2GroundNetDevice = netDevice->GetObject<Sat2GroundNetDevice> ();

      Address address = sat2GroundNetDevice->GetAddress ();
      SatAddress satAddress = SatAddress::ConvertFrom (address);
      if (constellationId != 0)
        {
          NS_ASSERT (constellationId == satAddress.getConstellationId ());
        }
      else
        {
          constellationId = satAddress.getConstellationId ();
        }
    }

  Ptr<Constellation> constellation = chelper->GetConstellation ();
  NS_ASSERT_MSG (c.GetN () == constellation->GetSize (),
                 "We need a complete constellation before installing all ISL.");

  const auto nPlanes = constellation->GetNPlanes ();
  const auto nNodesPerPlane = constellation->GetPlaneSize ();

  for (uint32_t i = 0; i < nPlanes; ++i)
    {
      for (uint32_t j = 0; j < nNodesPerPlane; ++j)
        {
          Ptr<Sat2GroundNetDevice> nd1 = constellation->GetSatellite (i, j);
          Ptr<Node> n1 = nd1->GetNode ();
          // Install intra-plane links
          if (j < nNodesPerPlane - 1)
            {
              Ptr<Sat2GroundNetDevice> nd2 = constellation->GetSatellite (i, j + 1);
              Ptr<Node> n2 = nd2->GetNode ();
              devices.Add (Install (n1, n2));
            }
          else if (j == nNodesPerPlane - 1 && j != 0 && j != 1) // We avoid creating one loop
            // (in case of only one node per plane) and double links (in case of only two nodes per plane)
            {
              Ptr<Sat2GroundNetDevice> nd2 = constellation->GetSatellite (i, 0);
              Ptr<Node> n2 = nd2->GetNode ();
              devices.Add (Install (n1, n2));
            }
          // Install inter-plane links
          if (i < nPlanes - 1)
            {
              Ptr<Sat2GroundNetDevice> nd2 =
                  constellation->GetSatellite (i + 1, (j != 0) ? j - 1 : nNodesPerPlane - 1);
              Ptr<Node> n2 = nd2->GetNode ();
              devices.Add (Install (n1, n2));
            }
          else if (i == nPlanes - 1 && i != 0 &&
                   i != 1) // We avoid creating one loop (in case of only one plane) and
            // double links (in case of only two planes)
            {
              Ptr<Sat2GroundNetDevice> nd2 =
                  constellation->GetSatellite (0, (j != 0) ? j - 1 : nNodesPerPlane - 1);
              Ptr<Node> n2 = nd2->GetNode ();
              devices.Add (Install (n1, n2));
            }
        }
    }
  return devices;
}

NetDeviceContainer
ISLHelper::Install (const NodeContainer &c) const
{
  NS_LOG_FUNCTION (this << &c);

  NS_ASSERT (c.GetN () == 2);
  return Install (c.Get (0), c.Get (1));
}

NetDeviceContainer
ISLHelper::Install (Ptr<Node> a, Ptr<Node> b) const
{
  NS_LOG_FUNCTION (this << a << b);

  NetDeviceContainer devices;

  Ptr<Sat2SatChannel> channel = m_channelFactory.Create<Sat2SatChannel> ();
  channel->SetAttribute ("TxSuccess",
                         PointerValue (m_successModelFactory.Create<Sat2SatSuccessModel> ()));
  devices.Add (InstallPriv (a, channel));
  devices.Add (InstallPriv (b, channel));
  return devices;
}

Ptr<NetDevice>
ISLHelper::InstallPriv (Ptr<Node> node, Ptr<Sat2SatChannel> channel) const
{
  NS_LOG_FUNCTION (this << node << channel);

  Ptr<SatNetDevice> device = m_satNetDeviceFactory.Create<SatNetDevice> ();
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

void
ISLHelper::EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous,
                               bool explicitFilename)
{
  //
  // All of the Pcap enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type ISLHelper.
  //
  NS_LOG_FUNCTION (this << prefix << nd);
  Ptr<SatNetDevice> device = nd->GetObject<SatNetDevice> ();
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
  NS_LOG_FUNCTION (this << stream << prefix << nd);
  //
  // All of the ascii enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type SatNetDevice.
  //
  Ptr<SatNetDevice> device = nd->GetObject<SatNetDevice> ();
  if (device == nullptr)
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
