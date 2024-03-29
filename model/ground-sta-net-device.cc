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
#include "ns3/mac48-address.h"
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

  /**
   * Set the packet power
   * \param power packet power
   */
  void
  SetPower (double power)
  {
    m_power = power;
  }
  /**
   * Get the packet power
   * \return the packet power
   */
  double
  GetPower (void) const
  {
    return m_power;
  }

  void Print (std::ostream &os) const override;

private:
  SatAddress m_dst; //!< destination address
  uint16_t m_protocolNumber; //!< protocol number
  double m_power; //!< packet power
};

NS_OBJECT_ENSURE_REGISTERED (GroundSatTag);

TypeId
GroundSatTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GroundSatTag")
                          .SetParent<Tag> ()
                          .SetGroupName ("ICARUS")
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
  return 8 + 8 + 2 + 8;
}
void
GroundSatTag::Serialize (TagBuffer i) const
{
  uint8_t mac[6];
  m_dst.CopyTo (mac);
  i.Write (mac, 6);
  i.WriteU16 (m_protocolNumber);
  i.WriteDouble (m_power);
}
void
GroundSatTag::Deserialize (TagBuffer i)
{
  uint8_t mac[6];

  i.Read (mac, 6);
  m_dst.CopyFrom (mac);
  m_protocolNumber = i.ReadU16 ();
  m_power = i.ReadDouble ();
}

void
GroundSatTag::Print (std::ostream &os) const
{
  os << " dst=" << m_dst << " proto=" << m_protocolNumber << " power=" << m_power;
}
} // namespace

TypeId
GroundStaNetDevice::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::icarus::GroundStaNetDevice")
          .SetParent<IcarusNetDevice> ()
          .SetGroupName ("ICARUS")
          .AddConstructor<GroundStaNetDevice> ()
          .AddAttribute ("Address", "The link-layer address of this device",
                         Mac48AddressValue (Mac48Address ("00:00:00:00:00:00")),
                         MakeMac48AddressAccessor (&GroundStaNetDevice::m_localAddress),
                         MakeMac48AddressChecker ())
          .AddAttribute ("RemoteAddress", "The link-layer address of the remote satellite",
                         SatAddressValue (SatAddress (0, 0, 0)),
                         MakeSatAddressAccessor (&GroundStaNetDevice::m_remoteAddress),
                         MakeSatAddressChecker ())
          .AddAttribute ("MacModelTx", "The MAC protocol for transmitted frames", PointerValue (),
                         MakePointerAccessor (&GroundStaNetDevice::m_macModel),
                         MakePointerChecker<MacModel> ())
          .AddAttribute (
              "TxPower", "The transmission power for this device (in dBm)", DoubleValue (0),
              MakeDoubleAccessor (&IcarusNetDevice::SetTxPower, &IcarusNetDevice::GetTxPower),
              MakeDoubleChecker<double> ());

  return tid;
}

GroundStaNetDevice::~GroundStaNetDevice ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

bool
GroundStaNetDevice::Attach (const Ptr<GroundSatChannel> &channel)
{
  NS_LOG_FUNCTION (this << channel);

  channel->AddGroundDevice (this);
  SetChannel (channel);
  m_linkChangeCallbacks ();

  return true;
}

void
GroundStaNetDevice::ReceiveFromSat (const Ptr<Packet> &packet, DataRate bps, const Address &src,
                                    uint16_t protocolNumber, double rxPower)
{
  NS_LOG_FUNCTION (this << packet << bps << src << protocolNumber << rxPower);

  if (SatAddress::ConvertFrom (src) != m_remoteAddress)
    {
      NS_LOG_LOGIC ("Ignoring packet from non-tracked satellite:" << src);
      return;
    }

  m_phyRxBeginTrace (packet);
  Simulator::Schedule (bps.CalculateBytesTxTime (packet->GetSize ()),
                       &GroundStaNetDevice::ReceiveFromSatFinish, this, packet, src,
                       protocolNumber);
}

void
GroundStaNetDevice::ReceiveFromSatFinish (const Ptr<Packet> &packet, const Address &src,
                                          uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << src << protocolNumber);

  m_phyRxEndTrace (packet);
  m_snifferTrace (packet);
  m_macRxTrace (packet);

  if (m_promiscReceiveCallback.IsNull () != true)
    {
      m_promiscReceiveCallback (this, packet, protocolNumber, src, GetAddress (), PACKET_HOST);
    }
  m_receiveCallback (this, packet, protocolNumber, src);
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

  return false;
}

void
GroundStaNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);

  m_localAddress = Mac48Address::ConvertFrom (address);
}

Address
GroundStaNetDevice::GetAddress (void) const
{
  NS_LOG_FUNCTION (this);

  return m_localAddress;
}

Address
GroundStaNetDevice::GetRemoteAddress () const
{
  NS_LOG_FUNCTION (this);

  return m_remoteAddress.ConvertTo ();
}

void
GroundStaNetDevice::SetRemoteAddress (const Address &address)
{
  NS_LOG_FUNCTION (this << address);

  SetRemoteAddress (SatAddress::ConvertFrom (address));
}

void
GroundStaNetDevice::SetRemoteAddress (const SatAddress &address)
{
  NS_LOG_FUNCTION (this << address);

  if (m_remoteAddress != address)
    {
      remoteAddressChange (m_remoteAddress, address);
    }
  m_remoteAddress = address;
}

Address
GroundStaNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);

  return Address ();
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

  return Address ();
}

Address
GroundStaNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);

  return Address ();
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

  return true;
}

bool
GroundStaNetDevice::Send (Ptr<Packet> packet, const Address &, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << protocolNumber);
  NS_LOG_WARN ("The protocol number should really be transmitted in a header somehow");

  GroundSatTag tag;
  tag.SetDst (m_remoteAddress);
  tag.SetProto (protocolNumber);
  tag.SetPower (GetTxPower ());
  packet->AddPacketTag (tag);

  m_macTxTrace (packet);
  if (GetQueue ()->Enqueue (packet) == false)
    {
      m_macTxDropTrace (packet);
      return false;
    }

  if (m_txMachineState == IDLE)
    {
      TransmitStart ();
    }

  return true;
}

void
GroundStaNetDevice::TransmitStart ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_txMachineState == IDLE,
                 "Must be IDLE to begin transmission. Tx state is: " << m_txMachineState);

  m_txMachineState = BUSY;
  auto packet = GetQueue ()->Dequeue ();

  GroundSatTag tag;
  packet->PeekPacketTag (tag);
  const auto dst = tag.GetDst ();
  const auto proto = tag.GetProto ();
  const auto power = tag.GetPower ();

  m_macModel->Send (
      packet,
      [=] (void) -> Time {
        m_snifferTrace (packet);
        m_phyTxBeginTrace (packet);
        return GetInternalChannel ()->Transmit2Sat (
            packet, GetDataRate (), GetObject<GroundStaNetDevice> (), dst, proto, power);
      },
      [=] (void) {
        m_phyTxEndTrace (packet);
        TransmitComplete (packet);
      });
}

void
GroundStaNetDevice::TransmitComplete (const Ptr<Packet> &packet)
{
  NS_LOG_FUNCTION (this << packet);

  GroundSatTag tag;
  packet->RemovePacketTag (tag);
  m_txMachineState = IDLE;

  if (GetQueue ()->IsEmpty () == false)
    {
      TransmitStart ();
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

  return false;
}

bool
GroundStaNetDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);

  return false;
}

} // namespace icarus
} // namespace ns3
