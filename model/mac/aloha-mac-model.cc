/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021-2022 Universidade de Vigo
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
#include "ns3/double.h"

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
          .AddAttribute ("SlotDuration", "The duration of a slot (0 for unslotted Aloha)",
                         TimeValue (Seconds (0)), MakeTimeAccessor (&AlohaMacModel::m_slotDuration),
                         MakeTimeChecker ())
          .AddAttribute ("SirThreshold", "The SIR threshold in dB (no capture effect by default)",
                         DoubleValue (std::numeric_limits<double>::max ()),
                         MakeDoubleAccessor (&AlohaMacModel::m_sirThreshold),
                         MakeDoubleChecker<double> ());
  return tid;
}

AlohaMacModel::AlohaMacModel () : m_busyPeriodPacketUid (boost::none), m_busyPeriodCollision (false)
{
  NS_LOG_FUNCTION (this);
}

void
AlohaMacModel::Send (const Ptr<Packet> &packet, std::function<Time (void)> transmit_callback,
                     std::function<void (void)> finish_callback)
{
  NS_LOG_FUNCTION (this << packet << &transmit_callback << &finish_callback);

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
  Simulator::Schedule (time_to_next_slot, &AlohaMacModel::DoSend, this, packet, transmit_callback,
                       finish_callback);
}

void
AlohaMacModel::DoSend (const Ptr<Packet> &packet, std::function<Time (void)> transmit_callback,
                       std::function<void (void)> finish_callback) const
{
  NS_LOG_FUNCTION (this << packet << &transmit_callback << &finish_callback);

  Time tx_time = transmit_callback ();
  Simulator::Schedule (tx_time, &AlohaMacModel::FinishTransmission, this, finish_callback);
}

void
AlohaMacModel::FinishTransmission (std::function<void (void)> finish_callback) const
{
  NS_LOG_FUNCTION (this << &finish_callback);

  return finish_callback ();
}

void
AlohaMacModel::StartPacketRx (const Ptr<Packet> &packet, Time packet_tx_time, double rx_power,
                              std::function<void (void)> net_device_cb)
{
  NS_LOG_FUNCTION (this << packet << packet_tx_time << rx_power << &net_device_cb);

  Time now = Simulator::Now ();
  if (m_busyPeriodPacketUid && now < m_busyPeriodFinishTime)
    {
      m_busyPeriodCollision = true;
      m_busyPeriodInterferencePower += rx_power;
      NS_LOG_LOGIC ("Packet " << packet->GetUid () << " causes collision");
    }

  Time finish_tx_time = now + packet_tx_time;
  if (!m_busyPeriodPacketUid || finish_tx_time >= m_busyPeriodFinishTime)
    {
      m_busyPeriodPacketUid = packet->GetUid ();
      m_busyPeriodFinishTime = finish_tx_time;
      m_busyPeriodInterferencePower = rx_power;
      NS_LOG_LOGIC ("Updating busy period info: " << m_busyPeriodPacketUid.value () << " "
                                                  << m_busyPeriodFinishTime << " "
                                                  << m_busyPeriodInterferencePower);
    }

  Simulator::Schedule (packet_tx_time, &AlohaMacModel::FinishReception, this, packet, rx_power,
                       net_device_cb);
}

void
AlohaMacModel::FinishReception (const Ptr<Packet> &packet, double rx_power,
                                std::function<void (void)> net_device_cb)
{
  NS_LOG_FUNCTION (this << packet << rx_power << &net_device_cb);

  bool has_collided = false;
  if (m_busyPeriodCollision)
    {
      double sir = 2.0 * rx_power - m_busyPeriodInterferencePower;
      if (sir < m_sirThreshold)
        {
          has_collided = true;
        }
    }

  uint64_t packet_uid = packet->GetUid ();
  if (m_busyPeriodPacketUid == packet_uid)
    {
      m_busyPeriodPacketUid = boost::none;
      m_busyPeriodFinishTime = Simulator::Now ();
      m_busyPeriodCollision = false;
      m_busyPeriodInterferencePower = 0;
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
