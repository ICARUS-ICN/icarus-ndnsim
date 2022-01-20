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
#include "ns3/pointer.h"

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
      m_busyPeriodCollidedPackets (std::map<uint64_t, uint32_t> ()),
      m_activeBusyPeriods (std::vector<Ptr<BusyPeriod>> ()),
      m_activeReceivedPackets (std::map<uint64_t, Time> ())
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
CrdsaMacModel::CleanActiveBusyPeriods (Time limit_time)
{
  NS_LOG_FUNCTION (this << limit_time);

  if (!m_activeBusyPeriods.empty ())
    {
      for (auto i = 0u; i < m_activeBusyPeriods.size (); i++)
        {
          if (m_activeBusyPeriods[i]->GetFinishTime () > limit_time)
            {
              m_activeBusyPeriods.erase (m_activeBusyPeriods.begin (),
                                         m_activeBusyPeriods.begin () + i);
              break;
            }
        }
    }
}

void
CrdsaMacModel::CleanActiveReceivedPackets (Time limit_time)
{
  NS_LOG_FUNCTION (this << limit_time);

  if (!m_activeReceivedPackets.empty ())
    {
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
}

std::map<uint64_t, uint32_t>
CrdsaMacModel::MakeInterferenceCancellation (void)
{
  NS_LOG_FUNCTION (this);

  std::map<uint64_t, uint32_t> recoveredPackets;

  if (!m_activeBusyPeriods.empty ())
    {
      auto it = m_activeBusyPeriods.begin ();
      while (it != m_activeBusyPeriods.end ())
        {
          for (auto collided : (*it)->GetCollidedPackets ())
            {
              auto received = m_activeReceivedPackets.find (collided.first);
              if (received != m_activeReceivedPackets.end ())
                {
                  (*it)->RemoveCollidedPacket (collided.first);
                }
            }
          auto n = (*it)->GetCollidedPackets ().size ();
          if (n == 0)
            {
              it = m_activeBusyPeriods.erase (it);
            }
          else
            {
              if (n == 1)
                {
                  auto recovered = (*it)->GetCollidedPackets ().begin ();
                  recoveredPackets.insert ({recovered->first, recovered->second});
                }
              ++it;
            }
        }
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
              std::cout << collided.first << " " << collided.second << " ";
            }
          std::cout << "}\n";
        }
    }
  //std::cout << "\n";
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
  //std::cout << "\n";
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

  m_busyPeriodCollidedPackets.insert ({packet->GetUid (), packet->GetSize ()});
  Simulator::Schedule (packet_tx_time, &CrdsaMacModel::FinishReception, this, packet,
                       net_device_cb);
}

void
CrdsaMacModel::FinishReception (const Ptr<Packet> &packet, std::function<void (void)> net_device_cb)
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

      m_activeReceivedPackets[packet_uid] = now;
      // Call the NetDevice for further processing
      net_device_cb ();
    }

  if (m_busyPeriodPacketUid == packet_uid)
    {
      if (has_collided)
        {
          m_activeBusyPeriods.push_back (
              CreateObject<BusyPeriod> (m_busyPeriodFinishTime, m_busyPeriodCollidedPackets));
        }
      Time limit_time = now - 2 * m_slotDuration * GetSlotsPerFrame ();
      CleanActiveBusyPeriods (limit_time);
      CleanActiveReceivedPackets (limit_time);
      PrintActiveBusyPeriods ();
      PrintActiveReceivedPackets ();

      std::map<uint64_t, uint32_t> recoveredPackets = MakeInterferenceCancellation ();
      while (!recoveredPackets.empty ())
        {
          for (auto const &recovered : recoveredPackets)
            {
              NS_LOG_LOGIC ("Packet " << recovered.first << " correctly recovered");

              m_activeReceivedPackets[recovered.first] = now;
              // Call the NetDevice for further processing
              // Pending...
            }
          recoveredPackets = MakeInterferenceCancellation ();
        }
      PrintActiveBusyPeriods ();
      PrintActiveReceivedPackets ();

      m_busyPeriodPacketUid = boost::none;
      m_busyPeriodFinishTime = now;
      m_busyPeriodCollision = false;
      m_busyPeriodCollidedPackets.clear ();
      NS_LOG_LOGIC ("Cleaning busy period info");
    }
}

} // namespace icarus
} // namespace ns3
