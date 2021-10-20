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
#include "ns3/net-device-container.h"
#include "ns3/net-device.h"

namespace ns3 {

class GroundStaNetDevice;
class Sat2GroundNetDevice;
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

  bool AttachNewSat (Ptr<Sat2GroundNetDevice> device);
  bool AttachGround (Ptr<GroundStaNetDevice> device);

  virtual std::size_t GetNDevices (void) const override;
  virtual Ptr<NetDevice> GetDevice (std::size_t i) const override;

private:
  NetDeviceContainer m_satellites;
  Ptr<NetDevice> m_ground = 0;
};
} // namespace ns3

#endif