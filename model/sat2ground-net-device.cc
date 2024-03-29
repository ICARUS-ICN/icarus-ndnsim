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
#include "ns3/assert.h"
#include "ns3/ground-sta-net-device.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/sat-address.h"
#include "ns3/pointer.h"
#include "ns3/uinteger.h"
#include "ns3/abort.h"
#include "ns3/simulator.h"

namespace ns3 {
namespace icarus {
NS_LOG_COMPONENT_DEFINE ("icarus.Sat2GroundNetDevice");

NS_OBJECT_ENSURE_REGISTERED (Sat2GroundNetDevice);

namespace {
/**
 * \brief Tag to store source, destination, protocol and power of each packet.
 */
class SatGroundTag : public Tag
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
  Mac48Address m_dst; //!< destination address
  uint16_t m_protocolNumber; //!< protocol number
  double m_power; //!< packet power
};

NS_OBJECT_ENSURE_REGISTERED (SatGroundTag);

TypeId
SatGroundTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SatGroundTag")
                          .SetParent<Tag> ()
                          .SetGroupName ("ICARUS")
                          .AddConstructor<SatGroundTag> ();
  return tid;
}
TypeId
SatGroundTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
SatGroundTag::GetSerializedSize (void) const
{
  return 8 + 8 + 2 + 8;
}
void
SatGroundTag::Serialize (TagBuffer i) const
{
  uint8_t mac[6];
  m_dst.CopyTo (mac);
  i.Write (mac, 6);
  i.WriteU16 (m_protocolNumber);
  i.WriteDouble (m_power);
}
void
SatGroundTag::Deserialize (TagBuffer i)
{
  uint8_t mac[6];

  i.Read (mac, 6);
  m_dst.CopyFrom (mac);
  m_protocolNumber = i.ReadU16 ();
  m_power = i.ReadDouble ();
}

void
SatGroundTag::Print (std::ostream &os) const
{
  os << " dst=" << m_dst << " proto=" << m_protocolNumber << " power=" << m_power;
}
} // namespace

TypeId
Sat2GroundNetDevice::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::icarus::Sat2GroundNetDevice")
          .SetParent<IcarusNetDevice> ()
          .SetGroupName ("ICARUS")
          .AddConstructor<Sat2GroundNetDevice> ()
          .AddAttribute (
              "Address", "The link-layer address of this device", SatAddressValue (SatAddress ()),
              MakeSatAddressAccessor (&Sat2GroundNetDevice::m_address), MakeSatAddressChecker ())
          .AddAttribute ("MacModelRx", "The MAC protocol for received frames", PointerValue (),
                         MakePointerAccessor (&Sat2GroundNetDevice::m_macModel),
                         MakePointerChecker<MacModel> ())
          .AddAttribute (
              "TxPower", "The transmission power for this device (in dBm)", DoubleValue (0),
              MakeDoubleAccessor (&IcarusNetDevice::SetTxPower, &IcarusNetDevice::GetTxPower),
              MakeDoubleChecker<double> ());

  return tid;
}

Sat2GroundNetDevice::~Sat2GroundNetDevice ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

bool
Sat2GroundNetDevice::Attach (const Ptr<GroundSatChannel> &channel)
{
  NS_LOG_FUNCTION (this << channel);

  SetChannel (channel);
  m_linkChangeCallbacks ();

  return true;
}

void
Sat2GroundNetDevice::ReceiveFromGround (const Ptr<Packet> &packet, DataRate bps, const Address &src,
                                        uint16_t protocolNumber, double rxPower)
{
  NS_LOG_FUNCTION (this << packet << bps << src << protocolNumber << rxPower);
  NS_ASSERT_MSG (m_macModel != nullptr, "Need a MacModel to receive packets.");

  m_phyRxBeginTrace (packet);
  Time packet_tx_time = bps.CalculateBytesTxTime (packet->GetSize ());

  m_macModel->StartPacketRx (packet, packet_tx_time, rxPower,
                             [=] { ReceiveFromGroundFinish (packet, src, protocolNumber); });
}

void
Sat2GroundNetDevice::ReceiveFromGroundFinish (const Ptr<Packet> &packet, const Address &src,
                                              uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << protocolNumber);

  m_phyRxEndTrace (packet);

  m_snifferTrace (packet);
  m_macRxTrace (packet);

  if (m_promiscReceiveCallback.IsNull () != true)
    {
      m_promiscReceiveCallback (this, packet, protocolNumber, src, GetAddress (), PACKET_HOST);
    }
  m_receiveCallback (this, packet, protocolNumber, src);
}

bool
Sat2GroundNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);

  NS_LOG_WARN ("Only to make the ARP implementation of ns3 happy.");
  // There should be a way to put a L2 address on a destination on a non-broadcast medium
  return true;
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

  SatGroundTag tag;
  tag.SetProto (protocolNumber);
  tag.SetPower (GetTxPower ());
  packet->AddPacketTag (tag);

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
      m_txMachineState = BUSY;
      TransmitStart ();
    }

  return true;
}

void
Sat2GroundNetDevice::TransmitStart ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_txMachineState == BUSY,
                 "Must be BUSY to transmit. Tx state is: " << m_txMachineState);

  auto packet = GetQueue ()->Dequeue ();
  m_snifferTrace (packet);

  SatGroundTag tag;
  packet->PeekPacketTag (tag);
  const auto proto = tag.GetProto ();
  const auto power = tag.GetPower ();

  m_phyTxBeginTrace (packet);
  GetInternalChannel ()->Transmit2Ground (packet, GetDataRate (), GetObject<Sat2GroundNetDevice> (),
                                          proto, power);
  Simulator::Schedule (GetDataRate ().CalculateBytesTxTime (packet->GetSize ()),
                       &Sat2GroundNetDevice::TransmitComplete, this, packet);
}

void
Sat2GroundNetDevice::TransmitComplete (const Ptr<Packet> &packet)
{
  NS_LOG_FUNCTION (this << packet);

  m_phyTxEndTrace (packet);

  SatGroundTag tag;
  packet->RemovePacketTag (tag);

  if (GetQueue ()->IsEmpty ())
    {
      m_txMachineState = IDLE;
    }
  else
    {
      TransmitStart ();
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
