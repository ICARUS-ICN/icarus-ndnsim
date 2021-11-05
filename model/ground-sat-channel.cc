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

#include "ground-sat-channel.h"
#include "ns3/assert.h"
#include "ns3/channel.h"
#include "ground-sat-success-model.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/mobility-model.h"
#include "ns3/net-device.h"
#include "ns3/pointer.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"
#include "sat2ground-net-device.h"
#include "ground-sta-net-device.h"
#include "ns3/assert.h"

namespace ns3 {
namespace icarus {
NS_LOG_COMPONENT_DEFINE ("icarus.GroundSatChannel");

NS_OBJECT_ENSURE_REGISTERED (GroundSatChannel);

TypeId
GroundSatChannel::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::icarus::GroundSatChannel")
          .SetParent<Channel> ()
          .SetGroupName ("ICARUS")
          .AddConstructor<GroundSatChannel> ()
          .AddAttribute ("TxSuccess",
                         "The object used to decide whether there is sufficient "
                         "visibility for a successful transmission",
                         PointerValue (), MakePointerAccessor (&GroundSatChannel::m_txSuccessModel),
                         MakePointerChecker<GroundSatSuccessModel> ())
          .AddTraceSource ("PhyTxDrop",
                           "Trace source indicating a packet has been "
                           "completely received by the device",
                           MakeTraceSourceAccessor (&GroundSatChannel::m_phyTxDropTrace),
                           "ns3::Packet::TracedCallback");

  return tid;
}

bool
GroundSatChannel::AttachNewSat (Ptr<Sat2GroundNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ABORT_MSG_UNLESS (device->GetNode ()->GetObject<MobilityModel> () != 0,
                       "Satellites need a mobility model");
  m_satellites.Add (device);

  return true;
}

bool
GroundSatChannel::AttachGround (Ptr<GroundStaNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ABORT_MSG_UNLESS (device->GetNode ()->GetObject<MobilityModel> () != 0,
                       "Ground stations need a mobility model");
  if (m_ground == 0)
    {
      m_ground = device;
      return true;
    }

  return false;
}

GroundSatChannel::GroundSatChannel () : Channel ()
{
  NS_LOG_FUNCTION (this);
}

GroundSatChannel::~GroundSatChannel ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

Time
GroundSatChannel::Transmit2Sat (Ptr<Packet> packet, DataRate bps, const SatAddress &dst,
                                uint16_t protocolNumber) const
{
  NS_LOG_FUNCTION (this << packet << bps << dst << protocolNumber);
  NS_ASSERT_MSG (m_ground, "We need a ground station");
  NS_ASSERT_MSG (m_satellites.GetN () == 1,
                 "WIP: Only 1 satellite en the constellation is supported");

  if (SatAddress::ConvertFrom (m_satellites.Get (0)->GetAddress ()) != dst)
    {
      NS_LOG_DEBUG ("Dropping packet as destination address is not in orbit");
      m_phyTxDropTrace (packet);
    }

  Time endTx = bps.CalculateBytesTxTime (packet->GetSize ());
  auto posGround = m_ground->GetNode ()->GetObject<MobilityModel> ();
  auto posSat = m_satellites.Get (0)->GetNode ()->GetObject<MobilityModel> ();
  auto distanceMeters = posGround->GetDistanceFrom (posSat);

  Time delay (Seconds (distanceMeters / 3e8));

  if (m_txSuccessModel != nullptr &&
      m_txSuccessModel->TramsmitSuccess (m_ground->GetNode (), m_satellites.Get (0)->GetNode (),
                                         packet) != true)
    {
      m_phyTxDropTrace (packet);
    }
  else
    {
      NS_ASSERT (DynamicCast<Sat2GroundNetDevice> (m_satellites.Get (0)) != 0);
      auto sat = StaticCast<Sat2GroundNetDevice> (m_satellites.Get (0));

      Simulator::ScheduleWithContext (sat->GetNode ()->GetId (), delay,
                                      &Sat2GroundNetDevice::ReceiveFromGround, sat, packet, bps,
                                      protocolNumber);
    }

  return endTx;
}

Time
GroundSatChannel::Transmit2Ground (Ptr<Packet> packet, DataRate bps, const SatAddress &src,
                                   uint16_t protocolNumber) const
{
  NS_LOG_FUNCTION (this << packet << bps << src << protocolNumber);
  NS_ASSERT_MSG (m_ground, "We need a ground station");
  NS_ASSERT_MSG (m_satellites.GetN () == 1,
                 "WIP: Only 1 satellite en the constellation is supported");

  Time endTx = bps.CalculateBytesTxTime (packet->GetSize ());
  auto posGround = m_ground->GetNode ()->GetObject<MobilityModel> ();
  auto posSat = m_satellites.Get (0)->GetNode ()->GetObject<MobilityModel> ();
  auto distanceMeters = posGround->GetDistanceFrom (posSat);

  Time delay (Seconds (distanceMeters / 3e8));

  if (m_txSuccessModel != nullptr &&
      m_txSuccessModel->TramsmitSuccess (m_ground->GetNode (), m_satellites.Get (0)->GetNode (),
                                         packet) != true)
    {
      NS_LOG_DEBUG ("Dropped packet " << packet);
      m_phyTxDropTrace (packet);
    }
  else
    {
      NS_ASSERT (DynamicCast<GroundStaNetDevice> (m_ground) != 0);

      Simulator::ScheduleWithContext (
          m_ground->GetNode ()->GetId (), delay, &GroundStaNetDevice::ReceiveFromSat,
          DynamicCast<GroundStaNetDevice> (m_ground), packet, bps, src, protocolNumber);
    }

  return endTx;
}

std::size_t
GroundSatChannel::GetNDevices (void) const
{
  NS_LOG_FUNCTION (this);
  return m_satellites.GetN () + (m_ground ? 1 : 0);
}

Ptr<NetDevice>
GroundSatChannel::GetDevice (std::size_t i) const
{
  NS_LOG_FUNCTION (this << i);
  // Last device is ground station
  NS_ABORT_MSG_UNLESS (i < GetNDevices (),
                       "Asking for " << i << "-th device of a total of " << GetNDevices ());

  if (i < m_satellites.GetN ())
    {
      return m_satellites.Get (i);
    }
  NS_ASSERT (i + 1 == GetNDevices ());
  NS_ASSERT (m_ground);

  return m_ground;
}
} // namespace icarus
} // namespace ns3
