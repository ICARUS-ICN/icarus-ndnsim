/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *
 * Copyright (c) 2022 Universidade de Vigo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Pablo Iglesias Sanuy <pabliglesias@alumnos.uvigo.es>
 *
 */

#include "sat2ground-transport.h"

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "model/ndn-block-header.hpp"
#include "ns3/ndnSIM/utils/ndn-ns3-packet-tag.hpp"

#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/data.hpp>

#include "ns3/queue.h"
#include "ns3/sat2ground-net-device.h"

NS_LOG_COMPONENT_DEFINE ("icarus.ndn.Sat2GroundTransport");

namespace ns3 {
namespace ndn {
namespace icarus {

Sat2GroundTransport::Sat2GroundTransport (Ptr<Node> node, const Ptr<NetDevice> &netDevice,
                                          const std::string &localUri, const std::string &remoteUri,
                                          ::ndn::nfd::FaceScope scope,
                                          ::ndn::nfd::FacePersistency persistency,
                                          ::ndn::nfd::LinkType linkType)
    : m_netDevice (DynamicCast<::ns3::icarus::Sat2GroundNetDevice> (netDevice)), m_node (node)
{
  this->setLocalUri (FaceUri (localUri));
  this->setRemoteUri (FaceUri (remoteUri));
  this->setScope (scope);
  this->setPersistency (persistency);
  this->setLinkType (linkType);
  this->setMtu (m_netDevice->GetMtu ()); // Use the MTU of the netDevice

  // Get send queue capacity for congestion marking
  PointerValue txQueueAttribute;
  if (m_netDevice->GetAttributeFailSafe ("TxQueue", txQueueAttribute))
    {
      Ptr<ns3::QueueBase> txQueue = txQueueAttribute.Get<ns3::QueueBase> ();
      // must be put into bytes mode queue

      auto size = txQueue->GetMaxSize ();
      if (size.GetUnit () == BYTES)
        {
          this->setSendQueueCapacity (size.GetValue ());
        }
      else
        {
          // don't know the exact size in bytes, guessing based on "standard" packet size
          this->setSendQueueCapacity (size.GetValue () * 1500);
        }
    }

  NS_LOG_FUNCTION (this << "Creating an ndnSIM transport instance for netDevice with URI"
                        << this->getLocalUri ());

  NS_ASSERT_MSG (m_netDevice != 0, "NetDeviceFace needs to be assigned a valid NetDevice");

  m_node->RegisterProtocolHandler (MakeCallback (&Sat2GroundTransport::receiveFromNetDevice, this),
                                   L3Protocol::ETHERNET_FRAME_TYPE, m_netDevice,
                                   true /*promiscuous mode*/);
}

Sat2GroundTransport::~Sat2GroundTransport ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

ssize_t
Sat2GroundTransport::getSendQueueLength ()
{
  PointerValue txQueueAttribute;
  if (m_netDevice->GetAttributeFailSafe ("TxQueue", txQueueAttribute))
    {
      Ptr<ns3::QueueBase> txQueue = txQueueAttribute.Get<ns3::QueueBase> ();
      return txQueue->GetNBytes ();
    }
  else
    {
      return nfd::face::QUEUE_UNSUPPORTED;
    }
}

void
Sat2GroundTransport::doClose ()
{
  NS_LOG_FUNCTION (this << "Closing transport for netDevice with URI" << this->getLocalUri ());

  // set the state of the transport to "CLOSED"
  this->setState (nfd::face::TransportState::CLOSED);
}

void
Sat2GroundTransport::doSend (const Block &packet, const nfd::EndpointId &endpoint)
{
  NS_LOG_FUNCTION (this << "Sending packet from netDevice with URI" << this->getLocalUri ());

  // convert NFD packet to NS3 packet
  BlockHeader header (packet);

  Ptr<ns3::Packet> ns3Packet = Create<ns3::Packet> ();
  ns3Packet->AddHeader (header);

  // send the NS3 packet
  m_netDevice->Send (ns3Packet, m_netDevice->GetBroadcast (), L3Protocol::ETHERNET_FRAME_TYPE);
}

// callback
void
Sat2GroundTransport::receiveFromNetDevice (Ptr<NetDevice> device, Ptr<const ns3::Packet> p,
                                           uint16_t protocol, const Address &from,
                                           const Address &to, NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION (device << p << protocol << from << to << packetType);

  // Convert NS3 packet to NFD packet
  Ptr<ns3::Packet> packet = p->Copy ();

  BlockHeader header;
  packet->RemoveHeader (header);

  this->receive (std::move (header.getBlock ()));
}

Ptr<NetDevice>
Sat2GroundTransport::GetNetDevice () const
{
  return m_netDevice;
}

} // namespace icarus
} // namespace ndn
} // namespace ns3
