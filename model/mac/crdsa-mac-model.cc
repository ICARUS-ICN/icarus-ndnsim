/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Universidade de Vigo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Sergio Herrería Alonso <sha@det.uvigo.es>
 */

#include "crdsa-mac-model.h"
#include "ns3/assert.h"
#include "ns3/log-macros-disabled.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include <algorithm>

namespace ns3 {
namespace icarus {

NS_LOG_COMPONENT_DEFINE ("icarus.CrdsaMacModel");

NS_OBJECT_ENSURE_REGISTERED (CrdsaMacModel);

TypeId
CrdsaMacModel::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::icarus::CrdsaMacModel")
          .SetParent<MacModel> ()
          .SetGroupName ("ICARUS")
          .AddConstructor<CrdsaMacModel> ()
          .AddAttribute ("SlotDuration", "The duration of a slot.", TimeValue (Seconds (0)),
                         MakeTimeAccessor (&CrdsaMacModel::m_slotDuration), MakeTimeChecker ())
          .AddAttribute ("SlotsPerFrame", "The number of slots in a frame.", UintegerValue (1),
                         MakeUintegerAccessor (&CrdsaMacModel::SetSlotsPerFrame,
                                               &CrdsaMacModel::GetSlotsPerFrame),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("ReplicasPerPacket", "The number of replicas per packet.",
                         UintegerValue (1),
                         MakeUintegerAccessor (&CrdsaMacModel::m_replicasPerPacket),
                         MakeUintegerChecker<uint16_t> ());

  return tid;
}

CrdsaMacModel::CrdsaMacModel () : m_busyPeriodPacketUid (boost::none), m_busyPeriodCollision (false)
{
  NS_LOG_FUNCTION (this);
}

uint16_t
CrdsaMacModel::NumReplicasPerPacket (void)
{
  NS_LOG_FUNCTION (this);

  return m_replicasPerPacket;
}

uint16_t
CrdsaMacModel::GetSlotsPerFrame () const
{
  NS_LOG_FUNCTION (this);

  return m_slotIds.size ();
}

void
CrdsaMacModel::SetSlotsPerFrame (uint16_t nSlots)
{
  NS_LOG_FUNCTION (this << nSlots);

  m_slotIds = std::vector<uint16_t> ();
  m_slotIds.reserve (nSlots);
  for (auto i = 0u; i < nSlots; i++)
    {
      m_slotIds.push_back (i);
    }

  NS_ASSERT (GetSlotsPerFrame () == nSlots);
}

std::vector<uint16_t>
CrdsaMacModel::GetSelectedSlots (void)
{
  NS_LOG_FUNCTION (this);

  std::random_shuffle (m_slotIds.begin (), m_slotIds.end ());
  std::vector<uint16_t> selectedSlots (m_slotIds.begin (),
                                       m_slotIds.begin () + NumReplicasPerPacket ());

  return selectedSlots;
}

void
CrdsaMacModel::Send (const Ptr<Packet> &packet, std::function<Time (void)> transmit_callback,
                     std::function<void (void)> finish_callback)
{
  NS_LOG_FUNCTION (this << packet << &transmit_callback << &finish_callback);

  Time time_to_next_frame = Seconds (0);
  if (GetSlotsPerFrame () > 0 && m_slotDuration.IsStrictlyPositive ())
    {
      Time now = Simulator::Now ();
      int64x64_t frame = now / (m_slotDuration * GetSlotsPerFrame ());
      // If fractional part of frame is 0, then we are already at the beginning of the next frame
      if (frame.GetLow () > 0)
        {
          time_to_next_frame = (frame.GetHigh () + 1) * m_slotDuration * GetSlotsPerFrame () - now;
        }
    }
  NS_LOG_LOGIC ("Time until the next frame: " << time_to_next_frame);

  for (auto slot : GetSelectedSlots ())
    {
      Time time_to_next_slot = time_to_next_frame + slot * m_slotDuration;
      Simulator::Schedule (time_to_next_slot, &CrdsaMacModel::StartPacketTx, this, packet,
                           transmit_callback, finish_callback);
      NS_LOG_LOGIC ("Time until the next slot " << slot << ": " << time_to_next_slot);
    }
}

void
CrdsaMacModel::StartPacketTx (const Ptr<Packet> &packet,
                              std::function<Time (void)> transmit_callback,
                              std::function<void (void)> finish_callback) const
{
  NS_LOG_FUNCTION (this << packet << &transmit_callback << &finish_callback);

  Time tx_time = transmit_callback ();
  Simulator::Schedule (tx_time, &CrdsaMacModel::FinishTransmission, this, finish_callback);
}

void
CrdsaMacModel::FinishTransmission (std::function<void (void)> finish_callback) const
{
  NS_LOG_FUNCTION (this << &finish_callback);

  return finish_callback ();
}

void
CrdsaMacModel::StartPacketRx (const Ptr<Packet> &packet, Time packet_tx_time,
                              std::function<void (void)> net_device_cb)
{
  NS_LOG_FUNCTION (this << packet << packet_tx_time << &net_device_cb);

  Time now = Simulator::Now ();
  if (m_busyPeriodPacketUid && now < m_busyPeriodFinishTime)
    {
      m_busyPeriodCollision = true;
      NS_LOG_LOGIC ("Packet " << packet->GetUid () << " causes collision");
    }

  Time finish_tx_time = now + packet_tx_time;
  if (!m_busyPeriodPacketUid || finish_tx_time >= m_busyPeriodFinishTime)
    {
      m_busyPeriodPacketUid = packet->GetUid ();
      m_busyPeriodFinishTime = finish_tx_time;
      NS_LOG_LOGIC ("Updating busy period info: " << m_busyPeriodPacketUid.value () << " "
                                                  << m_busyPeriodFinishTime);
    }

  Simulator::Schedule (packet_tx_time, &CrdsaMacModel::FinishReception, this, packet,
                       net_device_cb);
}

void
CrdsaMacModel::FinishReception (const Ptr<Packet> &packet, std::function<void (void)> net_device_cb)
{
  NS_LOG_FUNCTION (this << packet << &net_device_cb);

  bool has_collided = m_busyPeriodCollision;
  uint64_t packet_uid = packet->GetUid ();

  if (m_busyPeriodPacketUid == packet_uid)
    {
      m_busyPeriodPacketUid = boost::none;
      m_busyPeriodFinishTime = Simulator::Now ();
      m_busyPeriodCollision = false;
      NS_LOG_LOGIC ("Cleaning busy period info");
    }

  if (has_collided)
    {
      NS_LOG_LOGIC ("Packet " << packet_uid << " discarded due to collision");
    }
  else
    {
      NS_LOG_LOGIC ("Packet " << packet_uid << " correctly received");

      // Call the NetDevice for further processing
      net_device_cb ();
    }
}

} // namespace icarus
} // namespace ns3
