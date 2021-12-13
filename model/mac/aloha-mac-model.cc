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
  static TypeId tid = TypeId ("ns3::icarus::AlohaMacModel")
                          .SetParent<MacModel> ()
                          .SetGroupName ("ICARUS")
                          .AddConstructor<AlohaMacModel> ();

  return tid;
}

AlohaMacModel::AlohaMacModel ()
{
  NS_LOG_FUNCTION (this);

  busy_period_packet_uid = 0;
  busy_period_collision = false;
}

void
AlohaMacModel::NewPacketRx (const Ptr<Packet> &packet, Time packet_tx_time)
{
  NS_LOG_FUNCTION (this << packet << packet_tx_time);

  if (busy_period_packet_uid > 0)
    {
      busy_period_collision = true;
      NS_LOG_LOGIC ("Packet " << packet->GetUid () << " causes collision");
    }

  Time finish_tx_time = Simulator::Now () + packet_tx_time;
  if (busy_period_packet_uid == 0 || finish_tx_time >= busy_period_finish_time)
    {
      busy_period_packet_uid = packet->GetUid ();
      busy_period_finish_time = finish_tx_time;
      NS_LOG_LOGIC ("Updating busy period info: " << busy_period_packet_uid << " "
                                                  << busy_period_finish_time);
    }
}

bool
AlohaMacModel::HasCollided (const Ptr<Packet> &packet)
{
  NS_LOG_FUNCTION (this << packet);

  bool has_collided = busy_period_collision;
  uint64_t packet_uid = packet->GetUid ();

  if (has_collided)
    {
      NS_LOG_LOGIC ("Packet " << packet_uid << " discarded due to collision");
    }
  else
    {
      NS_LOG_LOGIC ("Packet " << packet_uid << " correctly received");
    }

  if (busy_period_packet_uid == packet_uid)
    {
      busy_period_packet_uid = 0;
      busy_period_finish_time = Simulator::Now ();
      busy_period_collision = false;
      NS_LOG_LOGIC ("Cleaning busy period info");
    }
  return has_collided;
}

} // namespace icarus
} // namespace ns3
