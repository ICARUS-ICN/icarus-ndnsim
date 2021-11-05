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
#include "icarus-net-device.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/sat-address.h"
#include "ns3/pointer.h"
#include "ns3/uinteger.h"
#include "ns3/abort.h"
#include "ns3/simulator.h"

namespace ns3 {
namespace icarus {
NS_LOG_COMPONENT_DEFINE ("icarus.Sat2GroundNetDevice");

NS_OBJECT_ENSURE_REGISTERED (Sat2GroundNetDevice);

TypeId
Sat2GroundNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::icarus::Sat2GroundNetDevice")
                          .SetParent<IcarusNetDevice> ()
                          .SetGroupName ("ICARUS")
                          .AddConstructor<Sat2GroundNetDevice> ()
                          .AddAttribute ("Address", "The link-layer address of this device.",
                                         SatAddressValue (SatAddress ()),
                                         MakeSatAddressAccessor (&Sat2GroundNetDevice::m_address),
                                         MakeSatAddressChecker ());

  return tid;
}

Sat2GroundNetDevice::~Sat2GroundNetDevice ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

bool
Sat2GroundNetDevice::Attach (Ptr<GroundSatChannel> channel)
{
  NS_LOG_FUNCTION (this << channel);

  if (channel->AttachNewSat (this))
    {
      SetChannel (channel);
      m_linkChangeCallbacks ();
      return true;
    }

  return false;
}

void
Sat2GroundNetDevice::ReceiveFromGround (Ptr<Packet> packet, DataRate bps, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << bps << protocolNumber);

  m_phyRxBeginTrace (packet);
  Simulator::Schedule (bps.CalculateBytesTxTime (packet->GetSize ()),
                       &Sat2GroundNetDevice::ReceiveFromGroundFinish, this, packet, protocolNumber);
}

void
Sat2GroundNetDevice::ReceiveFromGroundFinish (Ptr<Packet> packet, uint16_t protocolNumber)
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

bool
Sat2GroundNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);

  return false;
}

void
Sat2GroundNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);

  m_address = SatAddress::ConvertFrom (address);
}

Address
Sat2GroundNetDevice::GetAddress (void) const
{
  NS_LOG_FUNCTION (this);

  return m_address.ConvertTo ();
}

Address
Sat2GroundNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG (LOG_WARN, "This is not supported");

  return Mac48Address::GetBroadcast ();
}

bool
Sat2GroundNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);

  return false;
}

Address
Sat2GroundNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  NS_LOG (LOG_WARN, "This is not supported");

  return Mac48Address::GetBroadcast ();
}

Address
Sat2GroundNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  NS_LOG (LOG_WARN, "This is not supported");

  return Mac48Address::GetBroadcast ();
}

bool
Sat2GroundNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);

  return false;
}

bool
Sat2GroundNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);

  return false;
}

bool
Sat2GroundNetDevice::Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
  NS_LOG_WARN ("The protocol number should really be transmitted in a header somehow");

  m_macTxTrace (packet);
  if (GetQueue ()->Enqueue (packet) == false)
    {
      m_macTxDropTrace (packet);
      return false;
    }

  NS_LOG_INFO (
      "Should we be able to perform simultaneous transmissions to DIFFERENT ground stations?");
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

void
Sat2GroundNetDevice::TransmitStart (Ptr<Packet> packet, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << protocolNumber);
  NS_ASSERT_MSG (m_txMachineState == IDLE,
                 "Must be IDLE to transmit. Tx state is: " << m_txMachineState);
  m_txMachineState = TRANSMITTING;

  m_phyTxBeginTrace (packet);
  Time endTx =
      GetInternalChannel ()->Transmit2Ground (packet, GetDataRate (), m_address, protocolNumber);
  Simulator::Schedule (endTx, &Sat2GroundNetDevice::TransmitComplete, this, packet, protocolNumber);
}

void
Sat2GroundNetDevice::TransmitComplete (Ptr<Packet> packet, uint16_t protocolNumber)
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
Sat2GroundNetDevice::SendFrom (Ptr<Packet> packet, const Address &source, const Address &dest,
                               uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNumber);
  NS_LOG (LOG_WARN, "This is not supported");

  return false;
}

bool
Sat2GroundNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);

  return false;
}

bool
Sat2GroundNetDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);

  return false;
}
} // namespace icarus
} // namespace ns3
