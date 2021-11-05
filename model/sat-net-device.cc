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
 * Author: Pablo Iglesias Sanuy <pabliglesias@alumnos.uvigo.es>
 *
 */

#include "sat-net-device.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/pointer.h"
#include "ns3/uinteger.h"
#include "ns3/abort.h"
#include "ns3/simulator.h"

namespace ns3 {
namespace icarus {
NS_LOG_COMPONENT_DEFINE ("icarus.SatNetDevice");

NS_OBJECT_ENSURE_REGISTERED (SatNetDevice);

TypeId
SatNetDevice::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::icarus::SatNetDevice")
          .SetParent<NetDevice> ()
          .SetGroupName ("ICARUS")
          .AddConstructor<SatNetDevice> ()
          .AddAttribute ("Channel", "The channel attached to this device", PointerValue (),
                         MakePointerAccessor (&SatNetDevice::m_channel),
                         MakePointerChecker<Sat2SatChannel> ())
          .AddAttribute (
              "DataRate", "The default data rate for ground<->satellite channels",
              DataRateValue (DataRate ("1Gb/s")),
              MakeDataRateAccessor (&SatNetDevice::SetDataRate, &SatNetDevice::GetDataRate),
              MakeDataRateChecker ())
          .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                         UintegerValue (DEFAULT_MTU),
                         MakeUintegerAccessor (&SatNetDevice::SetMtu, &SatNetDevice::GetMtu),
                         MakeUintegerChecker<uint16_t> ())
          //
          // Transmit queueing discipline for the device which includes its own set
          // of trace hooks.
          //
          .AddAttribute ("TxQueue", "A queue to use as the transmit queue in the device.",
                         PointerValue (),
                         MakePointerAccessor (&SatNetDevice::SetQueue, &SatNetDevice::GetQueue),
                         MakePointerChecker<Queue<Packet>> ()) //
          // Trace sources at the "top" of the net device, where packets transition
          // to/from higher layers.
          //
          .AddTraceSource ("MacTx",
                           "Trace source indicating a packet has "
                           "arrived for transmission by this device",
                           MakeTraceSourceAccessor (&SatNetDevice::m_macTxTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacTxDrop",
                           "Trace source indicating a packet has been "
                           "dropped by the device before transmission",
                           MakeTraceSourceAccessor (&SatNetDevice::m_macTxDropTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacRx",
                           "A packet has been received by this device, "
                           "has been passed up from the physical layer "
                           "and is being forwarded up the local protocol stack.  "
                           "This is a non-promiscuous trace,",
                           MakeTraceSourceAccessor (&SatNetDevice::m_macRxTrace),
                           "ns3::Packet::TracedCallback")
          //
          // Trace sources at the "bottom" of the net device, where packets transition
          // to/from the channel.
          //
          .AddTraceSource ("PhyRxBegin",
                           "Trace source indicating a packet has "
                           "begun being received by the device",
                           MakeTraceSourceAccessor (&SatNetDevice::m_phyRxBeginTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("PhyTxBegin",
                           "Trace source indicating a packet has "
                           "begun transmitting over the channel",
                           MakeTraceSourceAccessor (&SatNetDevice::m_phyTxBeginTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("PhyTxEnd",
                           "Trace source indicating a packet has been "
                           "completely transmitted over the channel",
                           MakeTraceSourceAccessor (&SatNetDevice::m_phyTxEndTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("PhyRxEnd",
                           "Trace source indicating a packet has been "
                           "completely received by the device",
                           MakeTraceSourceAccessor (&SatNetDevice::m_phyRxEndTrace),
                           "ns3::Packet::TracedCallback")
          //
          // Trace sources designed to simulate a packet sniffer facility (tcpdump).
          //
          .AddTraceSource ("Sniffer",
                           "Trace source simulating a non-promiscuous "
                           "packet sniffer attached to the device",
                           MakeTraceSourceAccessor (&SatNetDevice::m_snifferTrace),
                           "ns3::Packet::TracedCallback");

  return tid;
}

SatNetDevice::SatNetDevice () : m_txMachineState (IDLE), m_channel (0)
{
  NS_LOG_FUNCTION (this);
}

SatNetDevice::~SatNetDevice ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

DataRate
SatNetDevice::GetDataRate () const
{
  NS_LOG_FUNCTION (this);

  return m_bps;
}

void
SatNetDevice::SetDataRate (DataRate rate)
{
  NS_LOG_FUNCTION (this << rate);
  m_bps = rate;
}

void
SatNetDevice::TransmitStart (Ptr<Packet> packet, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << protocolNumber);
  NS_ASSERT_MSG (m_txMachineState == IDLE,
                 "Must be IDLE to transmit. Tx state is: " << m_txMachineState);
  m_txMachineState = TRANSMITTING;

  m_phyTxBeginTrace (packet);
  Time endTx = GetInternalChannel ()->TransmitStart (packet, this, GetDataRate (), protocolNumber);
  Simulator::Schedule (endTx, &SatNetDevice::TransmitComplete, this, packet, protocolNumber);
}

void
SatNetDevice::TransmitComplete (Ptr<Packet> packet, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << protocolNumber);

  m_phyTxEndTrace (packet);
  m_txMachineState = IDLE;

  if (GetQueue ()->IsEmpty () == false)
    {
      auto packet = GetQueue ()->Dequeue ();
      m_snifferTrace (packet);
      TransmitStart (packet, protocolNumber);
    }
}

bool
SatNetDevice::Attach (Ptr<Sat2SatChannel> channel)
{
  NS_LOG_FUNCTION (this << channel);

  if (channel->AttachNewSat (this))
    {
      m_channel = channel;
      m_linkChangeCallbacks ();
      return true;
    }

  return false;
}

Ptr<Queue<Packet>>
SatNetDevice::GetQueue () const
{
  NS_LOG_FUNCTION (this);

  return m_queue;
}

void
SatNetDevice::SetQueue (Ptr<Queue<Packet>> queue)
{
  NS_LOG_FUNCTION (this << queue);

  m_queue = queue;
}

void
SatNetDevice::Receive (Ptr<Packet> packet, DataRate bps, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << bps << protocolNumber);

  m_phyRxBeginTrace (packet);
  Simulator::Schedule (bps.CalculateBytesTxTime (packet->GetSize ()), &SatNetDevice::ReceiveFinish,
                       this, packet, protocolNumber);
}

void
SatNetDevice::ReceiveFinish (Ptr<Packet> packet, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << protocolNumber);

  m_phyRxEndTrace (packet);
  m_snifferTrace (packet);
  m_macRxTrace (packet);

  NS_LOG_WARN ("FIXME: Missing source address.");
  NS_LOG_WARN ("FIXME: Have to specify packet type properly");
  static auto macUnspecified = Mac48Address ("00:00:00:00:00:00");

  if (m_promiscReceiveCallback.IsNull () != true)
    {
      m_promiscReceiveCallback (this, packet, protocolNumber, macUnspecified, macUnspecified,
                                PACKET_HOST);
    }
  m_receiveCallback (this, packet, protocolNumber, macUnspecified);
}

void
SatNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this << index);

  m_ifIndex = index;
}

uint32_t
SatNetDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ifIndex;
}

Ptr<Channel>
SatNetDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION (this);

  return m_channel;
}

void
SatNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);

  NS_LOG (LOG_WARN, "This is not supported");
}

Address
SatNetDevice::GetAddress (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG (LOG_WARN, "This is not supported");

  return Mac48Address::GetBroadcast ();
}

bool
SatNetDevice::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);

  m_mtu = mtu;

  return true;
}

uint16_t
SatNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);

  return m_mtu;
}

bool
SatNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);

  return m_channel != 0;
}

void
SatNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION (this << &callback);

  m_linkChangeCallbacks.ConnectWithoutContext (callback);
}

bool
SatNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);

  return false;
}

Address
SatNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG (LOG_WARN, "This is not supported");

  return Mac48Address::GetBroadcast ();
}

bool
SatNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);

  return false;
}

Address
SatNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  NS_LOG (LOG_WARN, "This is not supported");

  return Mac48Address::GetBroadcast ();
}

Address
SatNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  NS_LOG (LOG_WARN, "This is not supported");

  return Mac48Address::GetBroadcast ();
}

bool
SatNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);

  return false;
}

bool
SatNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);

  return true;
}

bool
SatNetDevice::Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
  NS_LOG_WARN ("The protocol number should really be transmitted in a header somehow");

  m_macTxTrace (packet);
  if (GetQueue ()->Enqueue (packet) == false)
    {
      m_macTxDropTrace (packet);
      return false;
    }

  if (m_txMachineState == IDLE)
    {
      if (GetQueue ()->IsEmpty () == false)
        {
          auto packet = GetQueue ()->Dequeue ();
          m_snifferTrace (packet);
          TransmitStart (packet, protocolNumber);
        }
    }

  return true;
}

bool
SatNetDevice::SendFrom (Ptr<Packet> packet, const Address &source, const Address &dest,
                        uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNumber);
  NS_LOG (LOG_WARN, "This is not supported");

  return false;
}

Ptr<Node>
SatNetDevice::GetNode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_node;
}

void
SatNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);

  m_node = node;
}

bool
SatNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);

  return false;
}

void
SatNetDevice::SetReceiveCallback (ReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);

  m_receiveCallback = cb;
}

void
SatNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);

  m_promiscReceiveCallback = cb;
}

bool
SatNetDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);

  return false;
}

Ptr<Sat2SatChannel>
SatNetDevice::GetInternalChannel (void) const
{
  return m_channel;
}

} // namespace icarus
} // namespace ns3
