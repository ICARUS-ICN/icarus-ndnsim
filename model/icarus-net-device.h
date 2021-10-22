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

class IcarusNetDevice : public NetDevice
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual ~IcarusNetDevice ();

  virtual bool Attach (Ptr<GroundSatChannel> channel) = 0;

  virtual DataRate GetDataRate () const = 0;
  virtual void SetDataRate (DataRate rate) = 0;

  virtual Ptr<Queue<Packet>> GetQueue () const = 0;
  virtual void SetQueue (Ptr<Queue<Packet>> rate) = 0;

  virtual void SetIfIndex (const uint32_t index) override = 0;
  virtual uint32_t GetIfIndex (void) const override = 0;

  virtual Ptr<Channel> GetChannel (void) const override = 0;

  virtual void SetAddress (Address address) override = 0;
  virtual Address GetAddress (void) const override = 0;

  virtual bool SetMtu (const uint16_t mtu) override = 0;

  virtual uint16_t GetMtu (void) const override = 0;
  virtual bool IsLinkUp (void) const override = 0;

  virtual void AddLinkChangeCallback (Callback<void> callback) override = 0;
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
  virtual Ptr<Node> GetNode (void) const override = 0;
  virtual void SetNode (Ptr<Node> node) override = 0;
  virtual bool NeedsArp (void) const override = 0;

  virtual void SetReceiveCallback (ReceiveCallback cb) override = 0;
  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb) override = 0;
  virtual bool SupportsSendFrom (void) const override = 0;
};
} // namespace ns3

#endif