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

#include "ground-sta-net-device.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/uinteger.h"
#include "ns3/abort.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("GroundStaNetDevice");

NS_OBJECT_ENSURE_REGISTERED (GroundStaNetDevice);

TypeId
GroundStaNetDevice::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::GroundStaNetDevice")
          .SetParent<NetDevice> ()
          .SetGroupName ("ICARUS")
          .AddConstructor<GroundStaNetDevice> ()
          .AddAttribute ("Address", "The MAC address of this device.",
                         Mac48AddressValue (Mac48Address ("ff:ff:ff:ff:ff:ff")),
                         MakeMac48AddressAccessor (&GroundStaNetDevice::m_address),
                         MakeMac48AddressChecker ())
          .AddAttribute (
              "Mtu", "The MAC-level Maximum Transmission Unit", UintegerValue (DEFAULT_MTU),
              MakeUintegerAccessor (&GroundStaNetDevice::SetMtu, &GroundStaNetDevice::GetMtu),
              MakeUintegerChecker<uint16_t> ())
          //
          // Transmit queueing discipline for the device which includes its own set
          // of trace hooks.
          //
          .AddAttribute ("TxQueue", "A queue to use as the transmit queue in the device.",
                         PointerValue (), MakePointerAccessor (&GroundStaNetDevice::m_queue),
                         MakePointerChecker<Queue<Packet>> ())

          //
          // Trace sources at the "top" of the net device, where packets transition
          // to/from higher layers.
          //
          .AddTraceSource ("MacTx",
                           "Trace source indicating a packet has "
                           "arrived for transmission by this device",
                           MakeTraceSourceAccessor (&GroundStaNetDevice::m_macTxTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacTxDrop",
                           "Trace source indicating a packet has been "
                           "dropped by the device before transmission",
                           MakeTraceSourceAccessor (&GroundStaNetDevice::m_macTxDropTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacRx",
                           "A packet has been received by this device, "
                           "has been passed up from the physical layer "
                           "and is being forwarded up the local protocol stack.  "
                           "This is a non-promiscuous trace,",
                           MakeTraceSourceAccessor (&GroundStaNetDevice::m_macRxTrace),
                           "ns3::Packet::TracedCallback")
          //
          // Trace sources at the "bottom" of the net device, where packets transition
          // to/from the channel.
          //
          .AddTraceSource ("PhyTxBegin",
                           "Trace source indicating a packet has "
                           "begun transmitting over the channel",
                           MakeTraceSourceAccessor (&GroundStaNetDevice::m_phyTxBeginTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("PhyTxEnd",
                           "Trace source indicating a packet has been "
                           "completely transmitted over the channel",
                           MakeTraceSourceAccessor (&GroundStaNetDevice::m_phyTxEndTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("PhyRxEnd",
                           "Trace source indicating a packet has been "
                           "completely received by the device",
                           MakeTraceSourceAccessor (&GroundStaNetDevice::m_phyRxEndTrace),
                           "ns3::Packet::TracedCallback")
          //
          // Trace sources designed to simulate a packet sniffer facility (tcpdump).
          //
          .AddTraceSource ("Sniffer",
                           "Trace source simulating a non-promiscuous "
                           "packet sniffer attached to the device",
                           MakeTraceSourceAccessor (&GroundStaNetDevice::m_snifferTrace),
                           "ns3::Packet::TracedCallback");
  return tid;
}

GroundStaNetDevice::~GroundStaNetDevice ()
{
}

bool
GroundStaNetDevice::Attach (Ptr<GroundSatChannel> channel)
{
  if (channel->AttachGround (this))
    {
      m_channel = channel;
      m_linkChangeCallbacks ();

      return true;
    }

  return false;
}

void
GroundStaNetDevice::SetIfIndex (const uint32_t index)
{
  m_ifIndex = index;
}
uint32_t
GroundStaNetDevice::GetIfIndex (void) const
{
  return m_ifIndex;
}

Ptr<Channel>
GroundStaNetDevice::GetChannel (void) const
{
  return m_channel;
}

void
GroundStaNetDevice::SetAddress (Address address)
{
  m_address = Mac48Address::ConvertFrom (address);
}

Address
GroundStaNetDevice::GetAddress (void) const
{
  return m_address;
}

bool
GroundStaNetDevice::SetMtu (const uint16_t mtu)
{
  m_mtu = mtu;

  return true;
}

uint16_t
GroundStaNetDevice::GetMtu (void) const
{
  return m_mtu;
}

bool
GroundStaNetDevice::IsLinkUp (void) const
{
  return m_channel != 0;
}

void
GroundStaNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  m_linkChangeCallbacks.ConnectWithoutContext (callback);
}

bool
GroundStaNetDevice::IsBroadcast (void) const
{
  return false;
}

Address
GroundStaNetDevice::GetBroadcast (void) const
{
  NS_LOG (LOG_WARN, "This is not supported");

  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
GroundStaNetDevice::IsMulticast (void) const
{
  return false;
}

Address
GroundStaNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG (LOG_WARN, "This is not supported");

  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

Address
GroundStaNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG (LOG_WARN, "This is not supported");

  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
GroundStaNetDevice::IsBridge (void) const
{
  return false;
}

bool
GroundStaNetDevice::IsPointToPoint (void) const
{
  return false;
}

bool
GroundStaNetDevice::Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
  NS_ABORT_MSG ("Not implemented yet");
}

bool
GroundStaNetDevice::SendFrom (Ptr<Packet> packet, const Address &source, const Address &dest,
                              uint16_t protocolNumber)
{
  NS_LOG (LOG_WARN, "This is not supported");

  return false;
}

Ptr<Node>
GroundStaNetDevice::GetNode (void) const
{
  return m_node;
}

void
GroundStaNetDevice::SetNode (Ptr<Node> node)
{
  m_node = node;
}

bool
GroundStaNetDevice::NeedsArp (void) const
{
  return false;
}

void
GroundStaNetDevice::SetReceiveCallback (ReceiveCallback cb)
{
  m_receiveCallback = cb;
}

void
GroundStaNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  NS_LOG (LOG_WARN, "This is not supported");
}

bool
GroundStaNetDevice::SupportsSendFrom (void) const
{
  return false;
}
} // namespace ns3
