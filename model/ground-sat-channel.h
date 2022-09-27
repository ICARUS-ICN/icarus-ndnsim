/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *
 * Copyright (c) 2021 Universidade de Vigo
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
 * Author: Miguel Rodríguez Pérez <miguel@det.uvigo.gal>
 *
 */

#ifndef GROUND_SAT_CHANNEL_H
#define GROUND_SAT_CHANNEL_H

#include "ns3/channel.h"
#include "ns3/data-rate.h"
#include "ns3/net-device-container.h"
#include "ns3/net-device.h"
#include "ns3/sat-address.h"
#include "ns3/traced-callback.h"

namespace ns3 {

class Packet;
class PropagationDelayModel;

namespace icarus {

class GroundStaNetDevice;
class Sat2GroundNetDevice;
class GroundSatSuccessModel;
class Constellation;

class GroundSatChannel : public Channel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  GroundSatChannel ();
  virtual ~GroundSatChannel ();

  void AddGroundDevice (const Ptr<GroundStaNetDevice> &device);

  void Transmit2Ground (const Ptr<Packet> &packet, DataRate bps,
                        const Ptr<Sat2GroundNetDevice> &src, uint16_t protocolNumber) const;
  Time Transmit2Sat (const Ptr<Packet> &packet, DataRate bps, const Ptr<GroundStaNetDevice> &src,
                     const SatAddress &dst, uint16_t protocolNumber) const;

  virtual std::size_t GetNDevices (void) const override;
  virtual Ptr<NetDevice> GetDevice (std::size_t i) const override;

  void SetConstellation (const Ptr<Constellation> &constellation);
  Ptr<Constellation> GetConstellation () const;

private:
  std::vector<Ptr<GroundStaNetDevice>> m_ground;
  Ptr<GroundSatSuccessModel> m_txSuccessModel;
  Ptr<PropagationDelayModel> m_propDelayModel;
  Ptr<Constellation> m_constellation;

  TracedCallback<Ptr<const Packet>> m_phyTxDropTrace;
};
} // namespace icarus
} // namespace ns3

#endif