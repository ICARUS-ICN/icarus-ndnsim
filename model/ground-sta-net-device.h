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

#ifndef GROUND_STA_NET_DEVICE_H
#define GROUND_STA_NET_DEVICE_H

#include "ndn-cxx/util/signal/signal.hpp"
#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/sat-address.h"
#include "ns3/mac48-address.h"
#include "ns3/queue.h"
#include "ns3/ground-sat-channel.h"
#include "icarus-net-device.h"
#include "ns3/mac-model.h"
namespace ns3 {
namespace icarus {

class GroundStaNetDevice : public IcarusNetDevice
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual ~GroundStaNetDevice ();

  bool Attach (const Ptr<GroundSatChannel> &channel) override;

  void ReceiveFromSat (const Ptr<Packet> &packet, DataRate bps, const Address &src,
                       uint16_t protocolNumber, double rxPower);

  Address GetAddress () const override;
  void SetAddress (Address address) override;

  Address GetRemoteAddress () const;
  void SetRemoteAddress (const Address &address);
  void SetRemoteAddress (const SatAddress &address);

  virtual void AddLinkChangeCallback (Callback<void> callback) override;
  virtual bool IsBroadcast (void) const override;
  virtual Address GetBroadcast (void) const override;

  virtual bool IsMulticast (void) const override;

  virtual Address GetMulticast (Ipv4Address multicastGroup) const override;

  virtual Address GetMulticast (Ipv6Address addr) const override;

  virtual bool IsBridge (void) const override;

  virtual bool IsPointToPoint (void) const override;

  virtual bool Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber) override;
  virtual bool SendFrom (Ptr<Packet> packet, const Address &source, const Address &dest,
                         uint16_t protocolNumber) override;
  virtual bool NeedsArp (void) const override;

  virtual bool SupportsSendFrom (void) const override;

  /** \brief signals when remote address changes
   */
  ::ndn::util::signal::Signal<GroundStaNetDevice, const SatAddress & /*old*/,
                              const SatAddress & /*new*/>
      remoteAddressChange;

private:
  enum { IDLE, BUSY } m_txMachineState = IDLE;
  Ptr<MacModel> m_macModel;

  Mac48Address m_localAddress;
  SatAddress m_remoteAddress;

  void ReceiveFromSatFinish (const Ptr<Packet> &packet, const Address &src,
                             uint16_t protocolNumber);
  void TransmitStart ();
  void TransmitComplete (const Ptr<Packet> &packet);
};

} // namespace icarus
} // namespace ns3

#endif
