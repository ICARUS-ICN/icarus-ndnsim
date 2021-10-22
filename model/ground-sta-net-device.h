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

#include "ns3/node.h"
#include "ns3/net-device.h"
#include "ns3/mac48-address.h"
#include "ns3/queue.h"
#include "ns3/ground-sat-channel.h"
#include "ns3/icarus-net-device.h"
#include <cstdint>

namespace ns3 {

class GroundStaNetDevice : public IcarusNetDevice
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual ~GroundStaNetDevice ();

  bool Attach (Ptr<GroundSatChannel> channel) override;

  DataRate GetDataRate () const override;
  void SetDataRate (DataRate rate) override;

  Ptr<Queue<Packet>> GetQueue () const override;
  void SetQueue (Ptr<Queue<Packet>> rate) override;

  virtual void SetIfIndex (const uint32_t index) override;
  virtual uint32_t GetIfIndex (void) const override;

  virtual Ptr<Channel> GetChannel (void) const override;

  virtual void SetAddress (Address address) override;
  virtual Address GetAddress (void) const override;

  virtual bool SetMtu (const uint16_t mtu) override;

  virtual uint16_t GetMtu (void) const override;
  virtual bool IsLinkUp (void) const override;

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
  virtual Ptr<Node> GetNode (void) const override;
  virtual void SetNode (Ptr<Node> node) override;
  virtual bool NeedsArp (void) const override;

  virtual void SetReceiveCallback (ReceiveCallback cb) override;
  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb) override;
  virtual bool SupportsSendFrom (void) const override;

private:
  enum { IDLE, TRANSMITTING } m_txMachineState = IDLE;

  static constexpr uint16_t DEFAULT_MTU = 1500;

  DataRate m_bps;
  Mac48Address m_address;
  uint32_t m_ifIndex;
  Ptr<Queue<Packet>> m_queue = 0;
  Ptr<GroundSatChannel> m_channel = 0;
  Ptr<Node> m_node = 0;
  uint16_t m_mtu;
  TracedCallback<> m_linkChangeCallbacks;
  ReceiveCallback m_receiveCallback;

  TracedCallback<Ptr<const Packet>> m_macTxTrace, m_macTxDropTrace, m_macRxTrace, m_phyTxBeginTrace,
      m_phyTxEndTrace, m_phyRxBeginTrace, m_phyRxEndTrace, m_snifferTrace;

  void TransmitStart (Ptr<Packet> packet, uint16_t protocolNumber);
  void TransmitComplete (Ptr<Packet> packet, uint16_t protocolNumber);
};

} // namespace ns3

#endif