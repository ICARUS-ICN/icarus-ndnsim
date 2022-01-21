/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *
 * Copyright (c) 2022 Universidade de Vigo
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
 * Author: Sergio Herrería Alonso <sha@det.uvigo.es>
 *
 */

#ifndef BUSY_PERIOD_H
#define BUSY_PERIOD_H

#include "ns3/nstime.h"
#include "ns3/object.h"
#include <functional>
#include <map>

namespace ns3 {
namespace icarus {

class BusyPeriod : public Object
{
public:
  BusyPeriod (const Time &finish_time,
              const std::map<uint64_t, std::function<void (void)>> &collided_packets)
      : finishTime (finish_time), collidedPackets (collided_packets)
  {
  }

  Time
  GetFinishTime (void) const
  {
    return finishTime;
  }

  const std::map<uint64_t, std::function<void (void)>> &
  GetCollidedPackets (void) const
  {
    return collidedPackets;
  }

  bool
  RemoveCollidedPacket (uint64_t packet_uid)
  {
    auto removed = collidedPackets.erase (packet_uid);

    NS_ASSERT_MSG (removed == 1, "Packet to be removed from the busy period not found");

    return true;
  }

private:
  const Time finishTime;
  std::map<uint64_t, std::function<void (void)>> collidedPackets;
};
} // namespace icarus
} // namespace ns3

#endif
