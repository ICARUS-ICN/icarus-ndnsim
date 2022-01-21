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
#include "private/busy-period.h"
#include "private/replicas-distro-polynomial.h"
#include "ns3/assert.h"
#include "ns3/log-macros-disabled.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
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
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("ReplicasDistribution",
                         "The distribution of the number of replicas per packet.", PointerValue (),
                         MakePointerAccessor (&CrdsaMacModel::m_replicasDistribution),
                         MakePointerChecker<ReplicasDistroPolynomial> ());

  return tid;
}

CrdsaMacModel::CrdsaMacModel ()
    : m_busyPeriodPacketUid (boost::none),
      m_busyPeriodCollision (false),
      m_busyPeriodCollidedPackets (),
      m_activeBusyPeriods (),
      m_activeReceivedPackets ()
{
  NS_LOG_FUNCTION (this);
}

uint16_t
CrdsaMacModel::NumReplicasPerPacket (void)
{
  NS_LOG_FUNCTION (this);

  if (m_replicasDistribution)
    {
      return m_replicasDistribution->NumReplicasPerPacket ();
    }
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
CrdsaMacModel::Send (const Ptr<Packet> &packet, txPacketCallback transmit_callback,
                     rxPacketCallback finish_callback)
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
CrdsaMacModel::StartPacketTx (const Ptr<Packet> &packet, txPacketCallback transmit_callback,
                              rxPacketCallback finish_callback) const
{
  NS_LOG_FUNCTION (this << packet << &transmit_callback << &finish_callback);

  Time tx_time = transmit_callback ();
  Simulator::Schedule (tx_time, &CrdsaMacModel::FinishTransmission, this, finish_callback);
}

void
CrdsaMacModel::FinishTransmission (rxPacketCallback finish_callback) const
{
  NS_LOG_FUNCTION (this << &finish_callback);

  return finish_callback ();
}

void
CrdsaMacModel::CleanActiveBusyPeriods (Time limit_time)
{
  NS_LOG_FUNCTION (this << limit_time);

  m_activeBusyPeriods.erase (
      m_activeBusyPeriods.begin (),
      std::find_if (m_activeBusyPeriods.cbegin (), m_activeBusyPeriods.cend (),
                    [limit_time] (auto busy) { return busy->GetFinishTime () > limit_time; }));
}

void
CrdsaMacModel::CleanActiveReceivedPackets (Time limit_time)
{
  NS_LOG_FUNCTION (this << limit_time);

  for (auto it = m_activeReceivedPackets.cbegin (); it != m_activeReceivedPackets.cend ();)
    {
      if (it->second < limit_time)
        {
          it = m_activeReceivedPackets.erase (it);
        }
      else
        {
          ++it;
        }
    }
}

std::vector<std::pair<uint64_t, MacModel::rxPacketCallback>>
CrdsaMacModel::MakeInterferenceCancellation (void)
{
  NS_LOG_FUNCTION (this);

  std::vector<std::pair<uint64_t, rxPacketCallback>> recoveredPackets;

  std::vector<std::vector<Ptr<BusyPeriod>>::const_iterator> periods_remove;

  for (auto it = m_activeBusyPeriods.begin (); it != m_activeBusyPeriods.end (); it++)
    {
      std::vector<uint64_t> received;
      for (const auto &collided : (*it)->GetCollidedPackets ())
        {
          if (m_activeReceivedPackets.find (collided.first) != m_activeReceivedPackets.end ())
            {
              received.push_back (collided.first);
            }
        }
      for (auto colid : received)
        {
          (*it)->RemoveCollidedPacket (colid);
        }
      if ((*it)->GetCollidedPackets ().empty ())
        {
          periods_remove.push_back (it);
        }
      else if ((*it)->GetCollidedPackets ().size () == 1)
        {
          auto recovered = *(*it)->GetCollidedPackets ().cbegin ();
          recoveredPackets.push_back (recovered);
        }
    }

  for (auto &period : periods_remove)
    {
      m_activeBusyPeriods.erase (period);
    }

  return recoveredPackets;
}

void
CrdsaMacModel::PrintActiveBusyPeriods (void) const
{
  NS_LOG_FUNCTION (this);

  std::cout << "\n--> ActiveBusyPeriods (" << m_activeBusyPeriods.size () << ") :\n";
  if (m_activeBusyPeriods.empty ())
    {
      std::cout << " { void }\n";
    }
  else
    {
      for (auto const &bp : m_activeBusyPeriods)
        {
          std::cout << " { " << bp->GetFinishTime () << ": ";
          for (auto const &collided : bp->GetCollidedPackets ())
            {
              std::cout << collided.first << " " << &collided.second << " ";
            }
          std::cout << "}\n";
        }
    }
}

void
CrdsaMacModel::PrintActiveReceivedPackets (void) const
{
  std::cout << "\n--> ActiveReceivedPackets (" << m_activeReceivedPackets.size () << ") :\n";
  if (m_activeReceivedPackets.empty ())
    {
      std::cout << "{ void }\n";
    }
  else
    {
      for (auto const &received : m_activeReceivedPackets)
        {
          std::cout << " {" << received.first << ": " << received.second << " }\n";
        }
    }
}

void
CrdsaMacModel::StartPacketRx (const Ptr<Packet> &packet, Time packet_tx_time,
                              rxPacketCallback net_device_cb)
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

  m_busyPeriodCollidedPackets.insert ({packet->GetUid (), net_device_cb});
  Simulator::Schedule (packet_tx_time, &CrdsaMacModel::FinishReception, this, packet,
                       net_device_cb);
}

void
CrdsaMacModel::FinishReception (const Ptr<Packet> &packet, rxPacketCallback net_device_cb)
{
  NS_LOG_FUNCTION (this << packet << &net_device_cb);

  bool has_collided = m_busyPeriodCollision;
  uint64_t packet_uid = packet->GetUid ();
  Time now = Simulator::Now ();

  if (has_collided)
    {
      NS_LOG_LOGIC ("Packet " << packet_uid << " discarded due to collision");
    }
  else
    {
      NS_LOG_LOGIC ("Packet " << packet_uid << " correctly received");

      if (m_activeReceivedPackets.find (packet_uid) == m_activeReceivedPackets.end ())
        {
          // This is the first time this packet is correctly received
          m_activeReceivedPackets[packet_uid] = now;
          net_device_cb ();
        }
    }

  if (m_busyPeriodPacketUid == packet_uid)
    {
      if (has_collided)
        {
          // New busy period with collided packets
          m_activeBusyPeriods.push_back (
              CreateObject<BusyPeriod> (m_busyPeriodFinishTime, m_busyPeriodCollidedPackets));
        }
      Time limit_time = now - 2 * m_slotDuration * GetSlotsPerFrame ();
      CleanActiveBusyPeriods (limit_time);
      CleanActiveReceivedPackets (limit_time);
      PrintActiveBusyPeriods ();
      PrintActiveReceivedPackets ();

      // Check if any previously collided packet can be recovered
      auto recoveredPackets = MakeInterferenceCancellation ();
      while (!recoveredPackets.empty ())
        {
          for (auto const &recovered : recoveredPackets)
            {
              NS_LOG_LOGIC ("Packet " << recovered.first << " correctly recovered");

              m_activeReceivedPackets[recovered.first] = now;
              recovered.second ();
            }
          recoveredPackets = MakeInterferenceCancellation ();
        }
      PrintActiveBusyPeriods ();
      PrintActiveReceivedPackets ();

      NS_LOG_LOGIC ("Cleaning busy period info");

      m_busyPeriodPacketUid = boost::none;
      m_busyPeriodFinishTime = now;
      m_busyPeriodCollision = false;
      m_busyPeriodCollidedPackets.clear ();
    }
}

} // namespace icarus
} // namespace ns3
