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
 * Authors: Pablo Iglesias Sanuy <pabliglesias@alumnos.uvigo.es>
 *
 */

#include "sat2sat-channel.h"
#include "circular-orbit.h"
#include "ns3/assert.h"
#include "ns3/channel.h"
#include "sat2sat-success-model.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/mobility-model.h"
#include "ns3/net-device.h"
#include "ns3/pointer.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"
#include "sat-net-device.h"
#include "ns3/assert.h"

namespace ns3 {
namespace icarus {
NS_LOG_COMPONENT_DEFINE ("icarus.Sat2SatChannel");

NS_OBJECT_ENSURE_REGISTERED (Sat2SatChannel);

TypeId
Sat2SatChannel::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::icarus::Sat2SatChannel")
          .SetParent<Channel> ()
          .SetGroupName ("ICARUS")
          .AddConstructor<Sat2SatChannel> ()
          .AddAttribute ("TxSuccess",
                         "The object used to decide whether there is sufficient "
                         "visibility for a successful transmission",
                         PointerValue (), MakePointerAccessor (&Sat2SatChannel::m_txSuccessModel),
                         MakePointerChecker<Sat2SatSuccessModel> ())
          .AddTraceSource ("PhyTxDrop",
                           "Trace source indicating a packet has been "
                           "completely received by the device",
                           MakeTraceSourceAccessor (&Sat2SatChannel::m_phyTxDropTrace),
                           "ns3::Packet::TracedCallback");

  return tid;
}

Sat2SatChannel::Sat2SatChannel () : Channel (), m_nSatellites (0)
{
  NS_LOG_FUNCTION (this);
}

Sat2SatChannel::~Sat2SatChannel ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

bool
Sat2SatChannel::AttachNewSat (Ptr<SatNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ABORT_MSG_UNLESS (device->GetNode ()->GetObject<MobilityModel> () != 0,
                       "Satellites need a mobility model");
  NS_ASSERT_MSG (m_nSatellites < N_SATELLITES, "Only two satellites permitted");
  NS_ASSERT (device != 0);

  m_link[m_nSatellites++].m_src = device;
  //
  // If we have both satellites connected to the channel, then finish introducing
  // the two halves and set the links to IDLE.
  //
  if (m_nSatellites == N_SATELLITES)
    {
      m_link[0].m_dst = m_link[1].m_src;
      m_link[1].m_dst = m_link[0].m_src;
      m_link[0].m_state = IDLE;
      m_link[1].m_state = IDLE;
      m_txSuccessModel->CalcMaxDistance(m_link[0].m_dst->GetNode()->GetObject<CircularOrbitMobilityModel> ()->getRadius());
    }

  return true;
}

Time
Sat2SatChannel::TransmitStart (Ptr<Packet> packet, Ptr<SatNetDevice> src, DataRate bps, uint16_t protocolNumber) const
{
  NS_LOG_FUNCTION (this << packet << bps << protocolNumber);

  NS_ASSERT (m_link[0].m_state != INITIALIZING);
  NS_ASSERT (m_link[1].m_state != INITIALIZING);

  uint32_t wire = src == m_link[0].m_src ? 0 : 1;
  auto dst = m_link[wire].m_dst;

  Time endTx = bps.CalculateBytesTxTime (packet->GetSize ());
  auto posSrc = src->GetNode ()->GetObject<MobilityModel> ();
  auto posDst = dst->GetNode ()->GetObject<MobilityModel> ();
  auto distanceMeters = posSrc->GetDistanceFrom (posDst);

  Time delay (Seconds (distanceMeters / 3e8));

  if (m_txSuccessModel != nullptr &&
      m_txSuccessModel->TramsmitSuccess (src->GetNode (), dst->GetNode(),
                                         packet) != true)
    {
      m_phyTxDropTrace (packet);
    }
  else
    {
      Simulator::ScheduleWithContext (dst->GetNode ()->GetId (), delay,
                                      &SatNetDevice::Receive, dst, packet, bps,
                                      protocolNumber);
    }

  return endTx;
}

std::size_t
Sat2SatChannel::GetNDevices (void) const
{
  NS_LOG_FUNCTION (this);
  return m_nSatellites;
}

Ptr<NetDevice>
Sat2SatChannel::GetDevice (std::size_t i) const
{
  NS_LOG_FUNCTION (this << i);
  // Last device is ground station
  NS_ABORT_MSG_UNLESS (i < GetNDevices () -1 && i >= 0,
                       "Asking for " << i << "-th device of a total of " << GetNDevices ());
  return m_link[i].m_src;
}
} // namespace icarus
} // namespace ns3
