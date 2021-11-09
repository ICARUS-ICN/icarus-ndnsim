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
#include "ns3/address.h"
#include "ns3/icarus-net-device.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/pointer.h"
#include "ns3/sat-address.h"
#include "ns3/uinteger.h"
#include "ns3/abort.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {
namespace icarus {
NS_LOG_COMPONENT_DEFINE ("icarus.GroundStaNetDevice");

NS_OBJECT_ENSURE_REGISTERED (GroundStaNetDevice);

namespace {
/**
 * \brief Tag to store source, destination and protocol of each packet.
 */
class GroundSatTag : public Tag
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const override;

  virtual uint32_t GetSerializedSize (void) const override;
  virtual void Serialize (TagBuffer i) const override;
  virtual void Deserialize (TagBuffer i) override;

  /**
   * Set the destination address
   * \param dst destination address
   */
  void
  SetDst (const SatAddress &dst)
  {
    m_dst = dst;
  }
  /**
   * Get the destination address
   * \return the destination address
   */
  const SatAddress &
  GetDst (void) const
  {
    return m_dst;
  }

  /**
   * Set the protocol number
   * \param proto protocol number
   */
  void
  SetProto (uint16_t proto)
  {
    m_protocolNumber = proto;
  }
  /**
   * Get the protocol number
   * \return the protocol number
   */
  uint16_t
  GetProto (void) const
  {
    return m_protocolNumber;
  }

  void Print (std::ostream &os) const override;

private:
  SatAddress m_dst; //!< destination address
  uint16_t m_protocolNumber; //!< protocol number
};

NS_OBJECT_ENSURE_REGISTERED (GroundSatTag);

TypeId
GroundSatTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GroundSatTag")
                          .SetParent<Tag> ()
                          .SetGroupName ("Icarus")
                          .AddConstructor<GroundSatTag> ();
  return tid;
}
TypeId
GroundSatTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
GroundSatTag::GetSerializedSize (void) const
{
  return 8 + 8 + 2;
}
void
GroundSatTag::Serialize (TagBuffer i) const
{
  uint8_t mac[6];
  m_dst.CopyTo (mac);
  i.Write (mac, 6);
  i.WriteU16 (m_protocolNumber);
}
void
GroundSatTag::Deserialize (TagBuffer i)
{
  uint8_t mac[6];

  i.Read (mac, 6);
  m_dst.CopyFrom (mac);
  m_protocolNumber = i.ReadU16 ();
}

void
GroundSatTag::Print (std::ostream &os) const
{
  os << " dst=" << m_dst << " proto=" << m_protocolNumber;
}
} // namespace

TypeId
GroundStaNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::icarus::GroundStaNetDevice")
                          .SetParent<IcarusNetDevice> ()
                          .SetGroupName ("ICARUS")
                          .AddConstructor<GroundStaNetDevice> ()
                          .AddAttribute ("Address", "The link-layer address of this device.",
                                         Mac48AddressValue (Mac48Address ("00:00:00:00:00:00")),
                                         MakeMac48AddressAccessor (&GroundStaNetDevice::m_address),
                                         MakeMac48AddressChecker ());

  return tid;
}

GroundStaNetDevice::~GroundStaNetDevice ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

bool
GroundStaNetDevice::Attach (Ptr<GroundSatChannel> channel)
{
  NS_LOG_FUNCTION (this << channel);

  if (channel->AttachGround (this))
    {
      SetChannel (channel);
      m_linkChangeCallbacks ();

      return true;
    }

  return false;
}

void
GroundStaNetDevice::ReceiveFromSat (Ptr<Packet> packet, DataRate bps, const SatAddress &src,
                                    uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << bps << src << protocolNumber);

  m_phyRxBeginTrace (packet);
  Simulator::Schedule (bps.CalculateBytesTxTime (packet->GetSize ()),
                       &GroundStaNetDevice::ReceiveFromSatFinish, this, packet, src,
                       protocolNumber);
}

void
GroundStaNetDevice::ReceiveFromSatFinish (Ptr<Packet> packet, const SatAddress &src,
                                          uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << src << protocolNumber);

  m_phyRxEndTrace (packet);
  m_snifferTrace (packet);
  m_macRxTrace (packet);

  NS_LOG_WARN ("FIXME: Have to specify packet type properly");

  static auto macUnspecified = Mac48Address ("00:00:00:00:00:00");
  if (m_promiscReceiveCallback.IsNull () != true)
    {
      m_promiscReceiveCallback (this, packet, protocolNumber, src.ConvertTo (), macUnspecified,
                                PACKET_HOST);
    }
  m_receiveCallback (this, packet, protocolNumber, src.ConvertTo ());
}

void
GroundStaNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION (this << &callback);

  m_linkChangeCallbacks.ConnectWithoutContext (callback);
}

bool
GroundStaNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);

  NS_LOG_WARN ("Only to make the ARP implementation of ns3 happy.");
  // There should be a way to put a L2 address on a destination on a non-broadcast medium
  return true;
}

void
GroundStaNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);

  m_address = Mac48Address::ConvertFrom (address);
}

Address
GroundStaNetDevice::GetAddress (void) const
{
  NS_LOG_FUNCTION (this);

  return m_address;
}

Address
GroundStaNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG (LOG_WARN, "This is not supported");

  return Mac48Address::GetBroadcast ();
}

bool
GroundStaNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);

  return false;
}

Address
GroundStaNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  NS_LOG (LOG_WARN, "This is not supported");

  return Mac48Address::GetBroadcast ();
}

Address
GroundStaNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  NS_LOG (LOG_WARN, "This is not supported");

  return Mac48Address::GetBroadcast ();
}

bool
GroundStaNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);

  return false;
}

bool
GroundStaNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);

  return false;
}

bool
GroundStaNetDevice::Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
  NS_LOG_WARN ("The protocol number should really be transmitted in a header somehow");

  GroundSatTag tag;
  tag.SetDst (SatAddress::ConvertFrom (dest));
  tag.SetProto (protocolNumber);
  packet->AddPacketTag (tag);

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
          TransmitStart (packet);
        }
    }

  return true;
}

void
GroundStaNetDevice::TransmitStart (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  NS_ASSERT_MSG (m_txMachineState == IDLE,
                 "Must be IDLE to transmit. Tx state is: " << m_txMachineState);
  m_txMachineState = TRANSMITTING;

  GroundSatTag tag;
  packet->PeekPacketTag (tag);
  const auto dst = tag.GetDst ();
  const auto proto = tag.GetProto ();

  m_phyTxBeginTrace (packet);
  Time endTx = GetInternalChannel ()->Transmit2Sat (packet, GetDataRate (), dst, proto);
  Simulator::Schedule (endTx, &GroundStaNetDevice::TransmitComplete, this, packet);
}

void
GroundStaNetDevice::TransmitComplete (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);

  m_phyTxEndTrace (packet);
  m_txMachineState = IDLE;

  if (GetQueue ()->IsEmpty () == false)
    {
      auto packet = GetQueue ()->Dequeue ();
      m_snifferTrace (packet);
      TransmitStart (packet);
    }
}

bool
GroundStaNetDevice::SendFrom (Ptr<Packet> packet, const Address &source, const Address &dest,
                              uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNumber);
  NS_LOG (LOG_WARN, "This is not supported");

  return false;
}

bool
GroundStaNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);

  return true;
}

bool
GroundStaNetDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);

  return false;
}
} // namespace icarus
} // namespace ns3
