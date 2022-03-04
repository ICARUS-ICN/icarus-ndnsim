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
 * Author: Pablo Iglesias Sanuy <pabliglesias@alumnos.uvigo.es>
 *
 */

#ifndef SAT2GROUND_TRANSPORT_HPP
#define SAT2GROUND_TRANSPORT_HPP

#include "model/ndn-common.hpp"
#include "daemon/face/transport.hpp"

#include "ndn-cxx/encoding/nfd-constants.hpp"
#include "ns3/net-device.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/pointer.h"

#include "ns3/sat2ground-net-device.h"
#include "ns3/channel.h"

namespace ns3 {
namespace ndn {
namespace icarus {

class Sat2GroundTransport : public nfd::face::Transport
{
public:
  Sat2GroundTransport (
      Ptr<Node> node, const Ptr<NetDevice> &netDevice, const std::string &localUri,
      const std::string &remoteUri, ::ndn::nfd::FaceScope scope = ::ndn::nfd::FACE_SCOPE_NON_LOCAL,
      ::ndn::nfd::FacePersistency persistency = ::ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
      ::ndn::nfd::LinkType linkType = ::ndn::nfd::LINK_TYPE_AD_HOC);

  ~Sat2GroundTransport ();

  Ptr<NetDevice> GetNetDevice () const;

  virtual ssize_t getSendQueueLength () final;

private:
  virtual void doClose () override;

  virtual void doSend (const Block &packet, const nfd::EndpointId &endpoint) override;

  void receiveFromNetDevice (Ptr<NetDevice> device, Ptr<const ns3::Packet> p, uint16_t protocol,
                             const Address &from, const Address &to,
                             NetDevice::PacketType packetType);

  Ptr<::ns3::icarus::Sat2GroundNetDevice> m_netDevice; ///< \brief Smart pointer to NetDevice
  Ptr<Node> m_node;
};

} // namespace icarus
} // namespace ndn
} // namespace ns3

#endif // SAT2GROUND_TRANSPORT_HPP
