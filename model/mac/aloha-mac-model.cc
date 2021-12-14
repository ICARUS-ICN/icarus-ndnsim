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

#include "aloha-mac-model.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {
namespace icarus {

NS_LOG_COMPONENT_DEFINE ("icarus.AlohaMacModel");

NS_OBJECT_ENSURE_REGISTERED (AlohaMacModel);

TypeId
AlohaMacModel::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::icarus::AlohaMacModel")
          .SetParent<MacModel> ()
          .SetGroupName ("ICARUS")
          .AddConstructor<AlohaMacModel> ()
          .AddAttribute ("SlotDuration", "The duration of a slot (0 for unslotted Aloha).",
                         TimeValue (Seconds (0)), MakeTimeAccessor (&AlohaMacModel::m_slotDuration),
                         MakeTimeChecker ());
  return tid;
}

AlohaMacModel::AlohaMacModel () : m_busyPeriodPacketUid (0), m_busyPeriodCollision (false)
{
  NS_LOG_FUNCTION (this);
}

Time
AlohaMacModel::TimeToNextSlot ()
{
  NS_LOG_FUNCTION (this);

  Time time_to_next_slot = Seconds (0);
  if (m_slotDuration.IsStrictlyPositive ())
    {
      Time now = Simulator::Now ();
      int64x64_t slot = now / m_slotDuration;
      // If fractional part of slot is 0, then we are at the beginning of the next slot
      if (slot.GetLow () > 0)
        {
          time_to_next_slot = (slot.GetHigh () + 1) * m_slotDuration - now;
        }
    }
  NS_LOG_LOGIC ("Time until the next slot: " << time_to_next_slot);
  return time_to_next_slot;
}

void
AlohaMacModel::NewPacketRx (const Ptr<Packet> &packet, Time packet_tx_time)
{
  NS_LOG_FUNCTION (this << packet << packet_tx_time);

  Time now = Simulator::Now ();
  if (m_busyPeriodPacketUid > 0 && now < m_busyPeriodFinishTime)
    {
      m_busyPeriodCollision = true;
      NS_LOG_LOGIC ("Packet " << packet->GetUid () << " causes collision");
    }

  Time finish_tx_time = now + packet_tx_time;
  if (m_busyPeriodPacketUid == 0 || finish_tx_time >= m_busyPeriodFinishTime)
    {
      m_busyPeriodPacketUid = packet->GetUid ();
      m_busyPeriodFinishTime = finish_tx_time;
      NS_LOG_LOGIC ("Updating busy period info: " << m_busyPeriodPacketUid << " "
                                                  << m_busyPeriodFinishTime);
    }
}

bool
AlohaMacModel::HasCollided (const Ptr<Packet> &packet)
{
  NS_LOG_FUNCTION (this << packet);

  bool has_collided = m_busyPeriodCollision;
  uint64_t packet_uid = packet->GetUid ();

  if (has_collided)
    {
      NS_LOG_LOGIC ("Packet " << packet_uid << " discarded due to collision");
    }
  else
    {
      NS_LOG_LOGIC ("Packet " << packet_uid << " correctly received");
    }

  if (m_busyPeriodPacketUid == packet_uid)
    {
      m_busyPeriodPacketUid = 0;
      m_busyPeriodFinishTime = Simulator::Now ();
      m_busyPeriodCollision = false;
      NS_LOG_LOGIC ("Cleaning busy period info");
    }
  return has_collided;
}

} // namespace icarus
} // namespace ns3
