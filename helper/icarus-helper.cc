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
#include "src/icarus/model/ground-sat-channel.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("IcarusHelper");

IcarusHelper::IcarusHelper ()
{
  NS_LOG_FUNCTION (this);

  m_queueFactory.SetTypeId ("ns3::DropTailQueue<Packet>");
  m_channelFactory.SetTypeId ("ns3::GroundSatChannel");
  m_sat2GroundFactory.SetTypeId ("ns3::Sat2GroundNetDevice");
  m_groundStaFactory.SetTypeId ("ns3::GroundStaNetDevice");
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

} // namespace ns3
