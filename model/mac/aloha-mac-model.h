/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *
 * Copyright (c) 2021-2022 Universidade de Vigo
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

#ifndef ALOHA_MAC_MODEL_H
#define ALOHA_MAC_MODEL_H

#include "mac-model.h"
#include "ns3/nstime.h"

#include <boost/optional.hpp>

namespace ns3 {
namespace icarus {

class AlohaMacModel : public MacModel
{
public:
  static TypeId GetTypeId (void);
  AlohaMacModel ();

  virtual void Send (const Ptr<Packet> &packet, std::function<Time (void)> transmit_callback,
                     std::function<void (void)> finish_callback) override;
  virtual void StartPacketRx (const Ptr<Packet> &packet, Time packet_tx_time, double rx_power,
                              std::function<void (void)>) override;

private:
  Time m_slotDuration;
  double m_sirThreshold;
  boost::optional<uint64_t> m_busyPeriodPacketUid;
  Time m_busyPeriodFinishTime;
  bool m_busyPeriodCollision;
  double m_busyPeriodInterferencePower;

  void DoSend (const Ptr<Packet> &packet, std::function<Time (void)> transmit_callback,
               std::function<void (void)> finish_callback) const;
  void FinishTransmission (std::function<void (void)>) const;
  void FinishReception (const Ptr<Packet> &packet, double rx_power, std::function<void (void)>);
};

} // namespace icarus
} // namespace ns3

#endif
