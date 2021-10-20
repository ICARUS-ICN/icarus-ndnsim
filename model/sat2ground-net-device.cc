/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *
 * Copyright (c) 2021 Universidade de Vigo
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
 * Author: Miguel Rodríguez Pérez <miguel@det.uvigo.gal>
 *
 */

#include "sat2ground-net-device.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/uinteger.h"
#include "ns3/abort.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("Sat2GroundNetDevice");

NS_OBJECT_ENSURE_REGISTERED (Sat2GroundNetDevice);

TypeId
Sat2GroundNetDevice::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::Sat2GroundNetDevice")
          .SetParent<NetDevice> ()
          .SetGroupName ("ICARUS")
          .AddConstructor<Sat2GroundNetDevice> ()
          .AddAttribute ("Address", "The MAC address of this device.",
                         Mac48AddressValue (Mac48Address ("ff:ff:ff:ff:ff:ff")),
                         MakeMac48AddressAccessor (&Sat2GroundNetDevice::m_address),
                         MakeMac48AddressChecker ())
          .AddAttribute (
              "Mtu", "The MAC-level Maximum Transmission Unit", UintegerValue (DEFAULT_MTU),
              MakeUintegerAccessor (&Sat2GroundNetDevice::SetMtu, &Sat2GroundNetDevice::GetMtu),
              MakeUintegerChecker<uint16_t> ())
          //
          // Transmit queueing discipline for the device which includes its own set
          // of trace hooks.
          //
          .AddAttribute ("TxQueue", "A queue to use as the transmit queue in the device.",
                         PointerValue (), MakePointerAccessor (&Sat2GroundNetDevice::m_queue),
                         MakePointerChecker<Queue<Packet>> ())

          //
          // Trace sources at the "top" of the net device, where packets transition
          // to/from higher layers.
          //
          .AddTraceSource ("MacTx",
                           "Trace source indicating a packet has "
                           "arrived for transmission by this device",
                           MakeTraceSourceAccessor (&Sat2GroundNetDevice::m_macTxTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacTxDrop",
                           "Trace source indicating a packet has been "
                           "dropped by the device before transmission",
                           MakeTraceSourceAccessor (&Sat2GroundNetDevice::m_macTxDropTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacRx",
                           "A packet has been received by this device, "
                           "has been passed up from the physical layer "
                           "and is being forwarded up the local protocol stack.  "
                           "This is a non-promiscuous trace,",
                           MakeTraceSourceAccessor (&Sat2GroundNetDevice::m_macRxTrace),
                           "ns3::Packet::TracedCallback")
          //
          // Trace sources at the "bottom" of the net device, where packets transition
          // to/from the channel.
          //
          .AddTraceSource ("PhyTxBegin",
                           "Trace source indicating a packet has "
                           "begun transmitting over the channel",
                           MakeTraceSourceAccessor (&Sat2GroundNetDevice::m_phyTxBeginTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("PhyTxEnd",
                           "Trace source indicating a packet has been "
                           "completely transmitted over the channel",
                           MakeTraceSourceAccessor (&Sat2GroundNetDevice::m_phyTxEndTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("PhyRxEnd",
                           "Trace source indicating a packet has been "
                           "completely received by the device",
                           MakeTraceSourceAccessor (&Sat2GroundNetDevice::m_phyRxEndTrace),
                           "ns3::Packet::TracedCallback")
          //
          // Trace sources designed to simulate a packet sniffer facility (tcpdump).
          //
          .AddTraceSource ("Sniffer",
                           "Trace source simulating a non-promiscuous "
                           "packet sniffer attached to the device",
                           MakeTraceSourceAccessor (&Sat2GroundNetDevice::m_snifferTrace),
                           "ns3::Packet::TracedCallback");
  return tid;
}

Sat2GroundNetDevice::~Sat2GroundNetDevice ()
{
}

bool
Sat2GroundNetDevice::Attach (Ptr<GroundSatChannel> channel)
{
  if (channel->AttachNewSat (this))
    {
      m_channel = channel;
      m_linkChangeCallbacks ();
      return true;
    }

  return false;
}

void
Sat2GroundNetDevice::SetIfIndex (const uint32_t index)
{
  m_ifIndex = index;
}
uint32_t
Sat2GroundNetDevice::GetIfIndex (void) const
{
  return m_ifIndex;
}

Ptr<Channel>
Sat2GroundNetDevice::GetChannel (void) const
{
  return m_channel;
}

void
Sat2GroundNetDevice::SetAddress (Address address)
{
  m_address = Mac48Address::ConvertFrom (address);
}

Address
Sat2GroundNetDevice::GetAddress (void) const
{
  return m_address;
}

bool
Sat2GroundNetDevice::SetMtu (const uint16_t mtu)
{
  m_mtu = mtu;

  return true;
}

uint16_t
Sat2GroundNetDevice::GetMtu (void) const
{
  return m_mtu;
}

bool
Sat2GroundNetDevice::IsLinkUp (void) const
{
  return m_channel != 0;
}

void
Sat2GroundNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  m_linkChangeCallbacks.ConnectWithoutContext (callback);
}

bool
Sat2GroundNetDevice::IsBroadcast (void) const
{
  return false;
}

Address
Sat2GroundNetDevice::GetBroadcast (void) const
{
  NS_LOG (LOG_WARN, "This is not supported");

  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
Sat2GroundNetDevice::IsMulticast (void) const
{
  return false;
}

Address
Sat2GroundNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG (LOG_WARN, "This is not supported");

  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

Address
Sat2GroundNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG (LOG_WARN, "This is not supported");

  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
Sat2GroundNetDevice::IsBridge (void) const
{
  return false;
}

bool
Sat2GroundNetDevice::IsPointToPoint (void) const
{
  return false;
}

bool
Sat2GroundNetDevice::Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
  NS_ABORT_MSG ("Not implemented yet");
}

bool
Sat2GroundNetDevice::SendFrom (Ptr<Packet> packet, const Address &source, const Address &dest,
                               uint16_t protocolNumber)
{
  NS_LOG (LOG_WARN, "This is not supported");

  return false;
}

Ptr<Node>
Sat2GroundNetDevice::GetNode (void) const
{
  return m_node;
}

void
Sat2GroundNetDevice::SetNode (Ptr<Node> node)
{
  m_node = node;
}

bool
Sat2GroundNetDevice::NeedsArp (void) const
{
  return false;
}

void
Sat2GroundNetDevice::SetReceiveCallback (ReceiveCallback cb)
{
  m_receiveCallback = cb;
}

void
Sat2GroundNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  NS_LOG (LOG_WARN, "This is not supported");
}

bool
Sat2GroundNetDevice::SupportsSendFrom (void) const
{
  return false;
}
} // namespace ns3
