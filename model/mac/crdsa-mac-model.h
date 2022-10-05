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

#ifndef CRDSA_MAC_MODEL_H
#define CRDSA_MAC_MODEL_H

#include "mac-model.h"
#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"

#include <map>
#include <boost/optional.hpp>

namespace ns3 {
namespace icarus {

class BusyPeriod;

class ReplicasDistroPolynomial : public Object
{
public:
  ReplicasDistroPolynomial (const std::vector<double> &c)
      : coefficients (c), rng (CreateObject<UniformRandomVariable> ())
  {
    NS_ASSERT_MSG (c.size () >= 2, "There needs to be at least two coefficients.");
    NS_ASSERT_MSG (c[0] == 0.0, "The first coefficient must always be 0.");
  }

  uint16_t
  NumReplicasPerPacket (void) const
  {
    const double p = rng->GetValue ();
    double coeffSum = 0;

    for (auto n = 1u; n < coefficients.size (); n++)
      {
        coeffSum += coefficients[n];
        if (coefficients[n] > 0 && p < coeffSum)
          {
            return n;
          }
      }

    NS_ABORT_MSG ("Invalid number of replicas per packet");
  }

private:
  const std::vector<double> coefficients;
  Ptr<UniformRandomVariable> rng;
};

class CrdsaMacModel : public MacModel
{
public:
  static TypeId GetTypeId (void);
  CrdsaMacModel ();

  virtual void Send (const Ptr<Packet> &packet, txPacketCallback transmit_callback,
                     rxPacketCallback finish_callback) override;
  virtual void StartPacketRx (const Ptr<Packet> &packet, Time packet_tx_time, double rx_power,
                              rxPacketCallback cb) override;

  uint16_t GetSlotsPerFrame () const;
  void SetSlotsPerFrame (uint16_t nSlots);

private:
  Time m_slotDuration;
  uint16_t m_replicasPerPacket;
  Ptr<ReplicasDistroPolynomial> m_replicasDistribution;
  double m_sirThreshold;
  boost::optional<uint64_t> m_busyPeriodPacketUid;
  Time m_busyPeriodFinishTime;
  bool m_busyPeriodCollision;
  double m_busyPeriodInterferencePower;
  std::map<uint64_t, rxPacketCallback> m_busyPeriodCollidedPackets;
  std::vector<Ptr<BusyPeriod>> m_activeBusyPeriods;
  std::map<uint64_t, Time> m_activeReceivedPackets;
  std::vector<uint16_t> m_slotIds;
  Ptr<UniformRandomVariable> m_rng;

  uint16_t NumReplicasPerPacket (void);
  std::vector<uint16_t> GetSelectedSlots (void);
  void StartPacketTx (const Ptr<Packet> &packet, txPacketCallback transmit_callback,
                      rxPacketCallback finish_callback) const;
  void FinishTransmission (rxPacketCallback cb) const;
  void FinishReception (const Ptr<Packet> &packet, double rx_power, rxPacketCallback cb);

  void CleanActiveBusyPeriods (Time limit_time);
  void CleanActiveReceivedPackets (Time limit_time);
  void PrintActiveBusyPeriods (void) const;
  void PrintActiveReceivedPackets (void) const;
  std::vector<std::pair<uint64_t, rxPacketCallback>> MakeInterferenceCancellation (void);
};

} // namespace icarus
} // namespace ns3

#endif
