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

#ifndef ICARUS_NET_DEVICE_H
#define ICARUS_NET_DEVICE_H

#include "ns3/net-device.h"
#include "ns3/data-rate.h"
#include "ns3/queue.h"
#include "ns3/ground-sat-channel.h"

namespace ns3 {
namespace icarus {

class IcarusNetDevice : public NetDevice
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual ~IcarusNetDevice ();

  virtual bool Attach (const Ptr<GroundSatChannel> &channel) = 0;

  virtual DataRate GetDataRate () const;
  virtual void SetDataRate (DataRate rate);

  virtual Ptr<Queue<Packet>> GetQueue () const;
  virtual void SetQueue (Ptr<Queue<Packet>> rate);

  virtual void SetIfIndex (const uint32_t index) override;
  virtual uint32_t GetIfIndex (void) const override;

  virtual Ptr<Channel> GetChannel (void) const override;

  virtual void SetAddress (Address address) override = 0;
  virtual Address GetAddress (void) const override = 0;

  virtual bool SetMtu (const uint16_t mtu) override;
  virtual uint16_t GetMtu (void) const override;
  virtual bool IsLinkUp (void) const override;

  virtual void AddLinkChangeCallback (Callback<void> callback) override;

  virtual bool IsBroadcast (void) const override = 0;
  virtual Address GetBroadcast (void) const override = 0;

  virtual bool IsMulticast (void) const override = 0;

  virtual Address GetMulticast (Ipv4Address multicastGroup) const override = 0;

  virtual Address GetMulticast (Ipv6Address addr) const override = 0;

  virtual bool IsBridge (void) const override = 0;

  virtual bool IsPointToPoint (void) const override = 0;

  virtual bool Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber) override = 0;
  virtual bool SendFrom (Ptr<Packet> packet, const Address &source, const Address &dest,
                         uint16_t protocolNumber) override = 0;
  virtual Ptr<Node> GetNode (void) const override;
  virtual void SetNode (Ptr<Node> node) override;
  virtual bool NeedsArp (void) const override = 0;

  virtual void SetReceiveCallback (ReceiveCallback cb) override;
  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb) override;
  virtual bool SupportsSendFrom (void) const override = 0;

protected:
  TracedCallback<> m_linkChangeCallbacks;
  TracedCallback<Ptr<const Packet>> m_macTxTrace, m_macTxDropTrace, m_macRxTrace, m_phyTxBeginTrace,
      m_phyTxEndTrace, m_phyRxBeginTrace, m_phyRxEndTrace, m_snifferTrace;
  PromiscReceiveCallback m_promiscReceiveCallback;
  ReceiveCallback m_receiveCallback;

  void SetChannel (Ptr<GroundSatChannel> channel);
  Ptr<GroundSatChannel> GetInternalChannel (void) const;

private:
  DataRate m_bps;
  uint32_t m_ifIndex;
  Ptr<Queue<Packet>> m_queue;
  Ptr<GroundSatChannel> m_channel;
  Ptr<Node> m_node;
  uint16_t m_mtu;
  static constexpr uint16_t DEFAULT_MTU = 1500;
};
} // namespace icarus
} // namespace ns3

#endif