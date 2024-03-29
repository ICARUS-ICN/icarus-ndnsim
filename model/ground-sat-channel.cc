/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *
 * Copyright (c) 2021–2022 Universidade de Vigo
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

#include "ns3/abort.h"
#include "ns3/constellation.h"
#include "ns3/ground-sat-success-model.h"
#include "ns3/ground-sta-net-device.h"
#include "ns3/ipv6-address.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/mobility-model.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/sat2ground-net-device.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"

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
          .AddAttribute ("PropDelayModel", "Object used to calculate the propagation delay",
                         PointerValue (), MakePointerAccessor (&GroundSatChannel::m_propDelayModel),
                         MakePointerChecker<PropagationDelayModel> ())
          .AddAttribute ("PropLossModel", "Object used to model the propagation loss",
                         PointerValue (), MakePointerAccessor (&GroundSatChannel::m_propLossModel),
                         MakePointerChecker<PropagationLossModel> ())
          .AddTraceSource ("PhyTxDrop",
                           "Trace source indicating a packet has been "
                           "dropped by the channel",
                           MakeTraceSourceAccessor (&GroundSatChannel::m_phyTxDropTrace),
                           "ns3::Packet::TracedCallback");

  return tid;
}

void
GroundSatChannel::AddGroundDevice (const Ptr<GroundStaNetDevice> &device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ABORT_MSG_UNLESS (device->GetNode ()->GetObject<MobilityModel> () != 0,
                       "Ground stations need a mobility model");

  m_ground.push_back (device);
}

GroundSatChannel::GroundSatChannel ()
    : Channel (), m_txSuccessModel (nullptr), m_constellation (nullptr)
{
  NS_LOG_FUNCTION (this);
}

GroundSatChannel::~GroundSatChannel ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

Time
GroundSatChannel::Transmit2Sat (const Ptr<Packet> &packet, DataRate bps,
                                const Ptr<GroundStaNetDevice> &src, const SatAddress &dst,
                                uint16_t protocolNumber, double txPower) const
{
  NS_LOG_FUNCTION (this << packet << bps << dst << protocolNumber << txPower);

  Time endTx = bps.CalculateBytesTxTime (packet->GetSize ());

  if (m_constellation == nullptr)
    {
      NS_LOG_WARN ("We need a constellation manager for transmissions to orbit.");
      m_phyTxDropTrace (packet);

      return endTx;
    }

  auto sat_device = m_constellation->GetSatellite (dst);
  if (sat_device == nullptr)
    {
      NS_LOG_DEBUG ("Dropping packet as destination address is not in orbit " << dst);
      m_phyTxDropTrace (packet);
    }

  const auto posGround = src->GetNode ()->GetObject<MobilityModel> ();
  const auto posSat = sat_device->GetNode ()->GetObject<MobilityModel> ();

  const Time delay = m_propDelayModel->GetDelay (posGround, posSat);
  const double rxPower = m_propLossModel->CalcRxPower (txPower, posGround, posSat);

  if (m_txSuccessModel != nullptr &&
      m_txSuccessModel->TramsmitSuccess (src->GetNode (), sat_device->GetNode (), packet) != true)
    {
      m_phyTxDropTrace (packet);
    }
  else
    {
      Simulator::ScheduleWithContext (sat_device->GetNode ()->GetId (), delay,
                                      &Sat2GroundNetDevice::ReceiveFromGround, sat_device, packet,
                                      bps, src->GetAddress (), protocolNumber, rxPower);
    }

  return endTx;
}

void
GroundSatChannel::Transmit2Ground (const Ptr<Packet> &packet, DataRate bps,
                                   const Ptr<Sat2GroundNetDevice> &src, uint16_t protocolNumber,
                                   double txPower) const
{
  NS_LOG_FUNCTION (this << packet << bps << &src << protocolNumber);

  for (const auto &ground_device : m_ground)
    {
      const auto posGround = ground_device->GetNode ()->GetObject<MobilityModel> ();
      const auto posSat = src->GetNode ()->GetObject<MobilityModel> ();

      const Time delay = m_propDelayModel->GetDelay (posGround, posSat);
      const double rxPower = m_propLossModel->CalcRxPower (txPower, posGround, posSat);

      if (m_txSuccessModel != nullptr &&
          m_txSuccessModel->TramsmitSuccess (ground_device->GetNode (), src->GetNode (), packet) !=
              true)
        {
          NS_LOG_DEBUG ("Dropped packet " << packet);
          m_phyTxDropTrace (packet);
        }
      else
        {
          Simulator::ScheduleWithContext (ground_device->GetNode ()->GetId (), delay,
                                          &GroundStaNetDevice::ReceiveFromSat, ground_device,
                                          packet, bps, src->GetAddress (), protocolNumber, rxPower);
        }
    }
}

std::size_t
GroundSatChannel::GetNDevices (void) const
{
  NS_LOG_FUNCTION (this);
  return m_constellation->GetSize () + m_ground.size ();
}

Ptr<NetDevice>
GroundSatChannel::GetDevice (std::size_t i) const
{
  NS_LOG_FUNCTION (this << i);
  // Last device is ground station
  NS_ABORT_MSG_UNLESS (i < GetNDevices (),
                       "Asking for " << i << "-th device of a total of " << GetNDevices ());

  if (i < m_constellation->GetSize ())
    {
      return m_constellation->Get (i);
    }

  return m_ground.at (i - m_constellation->GetSize ());
}

void
GroundSatChannel::SetConstellation (const Ptr<Constellation> &constellation)
{
  NS_LOG_FUNCTION (this << constellation);

  m_constellation = constellation;
}

Ptr<Constellation>
GroundSatChannel::GetConstellation () const
{
  NS_LOG_FUNCTION (this);

  return m_constellation;
}

} // namespace icarus
} // namespace ns3
