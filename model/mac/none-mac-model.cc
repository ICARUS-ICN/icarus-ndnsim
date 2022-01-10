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

#include "none-mac-model.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {
namespace icarus {

NS_LOG_COMPONENT_DEFINE ("icarus.NoneMacModel");

NS_OBJECT_ENSURE_REGISTERED (NoneMacModel);

TypeId
NoneMacModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::icarus::NoneMacModel")
                          .SetParent<MacModel> ()
                          .SetGroupName ("ICARUS")
                          .AddConstructor<NoneMacModel> ();

  return tid;
}

Time
NoneMacModel::TimeToNextSlot ()
{
  NS_LOG_FUNCTION (this);

  return Seconds (0);
}

void
NoneMacModel::StartPacketRx (const Ptr<Packet> &packet, Time packet_tx_time,
                             std::function<void (void)> net_device_cb)
{
  NS_LOG_FUNCTION (this << packet << packet_tx_time << &net_device_cb);

  Simulator::Schedule (packet_tx_time, &NoneMacModel::FinishReception, this, net_device_cb);
}

void
NoneMacModel::FinishReception (std::function<void (void)> net_device_cb) const
{
  NS_LOG_FUNCTION (this << &net_device_cb);

  return net_device_cb ();
}

} // namespace icarus
} // namespace ns3
