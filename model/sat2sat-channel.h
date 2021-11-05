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
 * Author: Pablo Iglesias Sanuy <pabliglesias@alumnos.uvigo.es>
 *
 */

#ifndef SAT2SAT_CHANNEL_H
#define SAT2SAT_CHANNEL_H

#include "ns3/channel.h"
#include "ns3/data-rate.h"
#include "ns3/sat2sat-success-model.h"
#include "ns3/net-device-container.h"
#include "ns3/net-device.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"

namespace ns3 {
namespace icarus {
class SatNetDevice;
class Sat2SatSuccessModel;
class Sat2SatChannel : public Channel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  Sat2SatChannel ();
  virtual ~Sat2SatChannel ();

  bool AttachNewSat (Ptr<SatNetDevice> device);

  Time TransmitStart (Ptr<Packet> packet, Ptr<SatNetDevice> src, DataRate bps, uint16_t protocolNumber) const;

  virtual std::size_t GetNDevices (void) const override;
  virtual Ptr<NetDevice> GetDevice (std::size_t i) const override;

private:
  static const std::size_t N_SATELLITES = 2;

  std::size_t m_nSatellites;
  Ptr<Sat2SatSuccessModel> m_txSuccessModel = nullptr;

  TracedCallback<Ptr<const Packet>> m_phyTxDropTrace;

    /** \brief Wire states
   *
   */
  enum WireState
  {
    /** Initializing state */
    INITIALIZING,
    /** Idle state (no transmission from NetDevice) */
    IDLE,
    /** Transmitting state (data being transmitted from NetDevice. */
    TRANSMITTING,
    /** Propagating state (data is being propagated in the channel. */
    PROPAGATING
  };

  /**
   * \brief Wire model for the PointToPointChannel
   */
  class Link
  {
public:
    /** \brief Create the link, it will be in INITIALIZING state
     *
     */
    Link() : m_state (INITIALIZING), m_src (0), m_dst (0) {}

    WireState                  m_state; //!< State of the link
    Ptr<SatNetDevice> m_src;   //!< First NetDevice
    Ptr<SatNetDevice> m_dst;   //!< Second NetDevice
  };

  Link m_link[N_SATELLITES];
};

} // namespace icarus
} // namespace ns3

#endif