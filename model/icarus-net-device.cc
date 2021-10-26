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

#include "icarus-net-device.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/pointer.h"
#include "ns3/node.h"
#include "ns3/ptr.h"
#include "ground-sat-channel.h"

namespace ns3 {
namespace icarus {
NS_LOG_COMPONENT_DEFINE ("icarus.IcarusNetDevice");

NS_OBJECT_ENSURE_REGISTERED (IcarusNetDevice);

TypeId
IcarusNetDevice::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::icarus::IcarusNetDevice")
          .SetParent<NetDevice> ()
          .SetGroupName ("ICARUS")
          .AddAttribute ("Channel", "The channel attached to this device", PointerValue (),
                         MakePointerAccessor (&IcarusNetDevice::m_channel),
                         MakePointerChecker<GroundSatChannel> ())
          .AddAttribute ("Address", "The MAC address of this device.",
                         Mac48AddressValue (Mac48Address ("00:00:00:00:00:00")),
                         MakeMac48AddressAccessor (&IcarusNetDevice::m_address),
                         MakeMac48AddressChecker ())
          .AddAttribute (
              "DataRate", "The default data rate for ground<->satellite channels",
              DataRateValue (DataRate ("1Gb/s")),
              MakeDataRateAccessor (&IcarusNetDevice::SetDataRate, &IcarusNetDevice::GetDataRate),
              MakeDataRateChecker ())
          .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                         UintegerValue (DEFAULT_MTU),
                         MakeUintegerAccessor (&IcarusNetDevice::SetMtu, &IcarusNetDevice::GetMtu),
                         MakeUintegerChecker<uint16_t> ())
          //
          // Transmit queueing discipline for the device which includes its own set
          // of trace hooks.
          //
          .AddAttribute (
              "TxQueue", "A queue to use as the transmit queue in the device.", PointerValue (),
              MakePointerAccessor (&IcarusNetDevice::SetQueue, &IcarusNetDevice::GetQueue),
              MakePointerChecker<Queue<Packet>> ()) //
          // Trace sources at the "top" of the net device, where packets transition
          // to/from higher layers.
          //
          .AddTraceSource ("MacTx",
                           "Trace source indicating a packet has "
                           "arrived for transmission by this device",
                           MakeTraceSourceAccessor (&IcarusNetDevice::m_macTxTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacTxDrop",
                           "Trace source indicating a packet has been "
                           "dropped by the device before transmission",
                           MakeTraceSourceAccessor (&IcarusNetDevice::m_macTxDropTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacRx",
                           "A packet has been received by this device, "
                           "has been passed up from the physical layer "
                           "and is being forwarded up the local protocol stack.  "
                           "This is a non-promiscuous trace,",
                           MakeTraceSourceAccessor (&IcarusNetDevice::m_macRxTrace),
                           "ns3::Packet::TracedCallback")
          //
          // Trace sources at the "bottom" of the net device, where packets transition
          // to/from the channel.
          //
          .AddTraceSource ("PhyRxBegin",
                           "Trace source indicating a packet has "
                           "begun being received by the device",
                           MakeTraceSourceAccessor (&IcarusNetDevice::m_phyRxBeginTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("PhyTxBegin",
                           "Trace source indicating a packet has "
                           "begun transmitting over the channel",
                           MakeTraceSourceAccessor (&IcarusNetDevice::m_phyTxBeginTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("PhyTxEnd",
                           "Trace source indicating a packet has been "
                           "completely transmitted over the channel",
                           MakeTraceSourceAccessor (&IcarusNetDevice::m_phyTxEndTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("PhyRxEnd",
                           "Trace source indicating a packet has been "
                           "completely received by the device",
                           MakeTraceSourceAccessor (&IcarusNetDevice::m_phyRxEndTrace),
                           "ns3::Packet::TracedCallback")
          //
          // Trace sources designed to simulate a packet sniffer facility (tcpdump).
          //
          .AddTraceSource ("Sniffer",
                           "Trace source simulating a non-promiscuous "
                           "packet sniffer attached to the device",
                           MakeTraceSourceAccessor (&IcarusNetDevice::m_snifferTrace),
                           "ns3::Packet::TracedCallback");

  return tid;
}

IcarusNetDevice::~IcarusNetDevice ()
{
  NS_LOG_FUNCTION (this);
}

DataRate
IcarusNetDevice::GetDataRate () const
{
  NS_LOG_FUNCTION (this);

  return m_bps;
}

void
IcarusNetDevice::SetDataRate (DataRate rate)
{
  NS_LOG_FUNCTION (this << rate);
  m_bps = rate;
}

Ptr<Queue<Packet>>
IcarusNetDevice::GetQueue () const
{
  NS_LOG_FUNCTION (this);

  return m_queue;
}

void
IcarusNetDevice::SetQueue (Ptr<Queue<Packet>> queue)
{
  NS_LOG_FUNCTION (this << queue);

  m_queue = queue;
}

void
IcarusNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this << index);

  m_ifIndex = index;
}

uint32_t
IcarusNetDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION (this);

  return m_ifIndex;
}

Ptr<Channel>
IcarusNetDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION (this);

  return m_channel;
}

void
IcarusNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);

  m_address = Mac48Address::ConvertFrom (address);
}

Address
IcarusNetDevice::GetAddress (void) const
{
  NS_LOG_FUNCTION (this);

  return m_address;
}

bool
IcarusNetDevice::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);

  m_mtu = mtu;

  return true;
}

uint16_t
IcarusNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);

  return m_mtu;
}

bool
IcarusNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);

  return m_channel != 0;
}

void
IcarusNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION (this << &callback);

  m_linkChangeCallbacks.ConnectWithoutContext (callback);
}

Ptr<Node>
IcarusNetDevice::GetNode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_node;
}

void
IcarusNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);

  m_node = node;
}

void
IcarusNetDevice::SetReceiveCallback (ReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);

  m_receiveCallback = cb;
}

void
IcarusNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  NS_LOG (LOG_WARN, "This is not supported");
}

void
IcarusNetDevice::SetChannel (Ptr<GroundSatChannel> channel)
{
  m_channel = channel;
}

Ptr<GroundSatChannel>
IcarusNetDevice::GetInternalChannel (void) const
{
  return m_channel;
}

} // namespace icarus
} // namespace ns3
