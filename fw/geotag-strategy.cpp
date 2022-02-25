/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2022 Universidade de Vigo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Pablo Iglesias Sanuy <pabliglesias@alumnos.uvigo.es>
 */

#include "geotag-strategy.hpp"
#include "common/logger.hpp"
#include "ns3/ground-sta-net-device.h"
#include "ns3/ndnSIM/NFD/daemon/fw/algorithm.hpp"
#include "ns3/ndnSIM/NFD/daemon/common/logger.hpp"
#include "ns3/ndnSIM/NFD/daemon/face/face-common.hpp"
#include "ns3/ndnSIM/NFD/daemon/face/face-endpoint.hpp"
#include "ns3/ndnSIM/ndn-cxx/lp/geo-tag.hpp"
#include "ns3/icarus-module.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/node-list.h"
#include "ns3/sat-address.h"
#include "ns3/sat-net-device.h"
#include "ns3/sat2ground-net-device.h"
#include "ns3/vector.h"
#include "ns3/ndnSIM/model/ndn-net-device-transport.hpp"
#include "ns3/ndnSIM/NFD/daemon/table/fib-nexthop.hpp"
#include "ns3/ndnSIM/NFD/daemon/face/transport.hpp"
#include "src/icarus/model/ground-sat-channel.h"
#include <boost/tuple/detail/tuple_basic.hpp>
#include <cstdint>
#include <string>
#include <tuple>

namespace nfd {
namespace fw {
using ns3::DynamicCast;

NFD_LOG_INIT (GeoTagStrategy);
NFD_REGISTER_STRATEGY (GeoTagStrategy);

const time::milliseconds GeoTagStrategy::RETX_SUPPRESSION_INITIAL (10);
const time::milliseconds GeoTagStrategy::RETX_SUPPRESSION_MAX (250);

GeoTagStrategy::GeoTagStrategy (Forwarder &forwarder, const Name &name)
    : Strategy (forwarder),
      ProcessNackTraits (this),
      m_retxSuppression (RETX_SUPPRESSION_INITIAL, RetxSuppressionExponential::DEFAULT_MULTIPLIER,
                         RETX_SUPPRESSION_MAX)
{
  ParsedInstanceName parsed = parseInstanceName (name);
  if (!parsed.parameters.empty ())
    {
      NDN_THROW (std::invalid_argument ("GeoTagStrategy does not accept parameters"));
    }
  if (parsed.version && *parsed.version != getStrategyName ()[-1].toVersion ())
    {
      NDN_THROW (std::invalid_argument ("GeoTagStrategy does not support version " +
                                        to_string (*parsed.version)));
    }
  this->setInstanceName (makeInstanceName (name, getStrategyName ()));

  auto node = ns3::NodeList::GetNode (ns3::NodeList::GetNNodes () - 1);
  auto constellation =
      DynamicCast<ns3::icarus::GroundSatChannel> (
          node->GetDevice (0)->GetObject<ns3::icarus::GroundStaNetDevice> ()->GetChannel ())
          ->GetConstellation ();
  this->m_nPlanes = constellation->GetNPlanes ();
  this->m_planeSize = constellation->GetPlaneSize ();
}

const Name &
GeoTagStrategy::getStrategyName ()
{
  static Name strategyName ("/localhost/nfd/strategy/geo-tag/%FD%01");
  return strategyName;
}

void
GeoTagStrategy::afterReceiveInterest (const FaceEndpoint &ingress, const Interest &interest,
                                      const shared_ptr<pit::Entry> &pitEntry)
{

  RetxSuppressionResult suppression = m_retxSuppression.decidePerPitEntry (*pitEntry);
  if (suppression == RetxSuppressionResult::SUPPRESS)
    {
      NFD_LOG_DEBUG (interest << " from=" << ingress << " suppressed");
      return;
    }

  const fib::Entry &fibEntry = this->lookupFib (*pitEntry);
  const fib::NextHopList &nexthops = fibEntry.getNextHops ();
  auto it = nexthops.end ();

  // Check if interest has geotag and node is a satellite
  auto geoTag = interest.getTag<ndn::lp::GeoTag> ();
  if (geoTag != nullptr && getNetDevice (ingress.face) != nullptr)
    {
      auto pos = geoTag->getPos ();
      auto coid = uint16_t (std::get<0> (pos));
      auto plane = uint16_t (std::get<1> (pos));
      auto pindex = uint16_t (std::get<2> (pos));
      auto ingressNetDevice = getNetDevice (ingress.face);

      auto sat2groundNetDevice = ingressNetDevice->GetNode ()
                                     ->GetDevice (0)
                                     ->GetObject<ns3::icarus::Sat2GroundNetDevice> ();
      auto this_address = ns3::icarus::SatAddress::ConvertFrom (sat2groundNetDevice->GetAddress ());
      auto coid_this = this_address.getConstellationId ();
      auto plane_this = this_address.getOrbitalPlane ();
      auto pindex_this = this_address.getPlaneIndex ();
      NFD_LOG_INFO ("INTEREST WITH GEOTAG (" << coid << ", " << plane << ", " << pindex
                                             << "), received in node (" << coid_this << ", "
                                             << plane_this << ", " << pindex_this << ")");

      // Get all eligible next-hops
      fib::NextHopList nhs;
      std::copy_if (nexthops.begin (), nexthops.end (), std::back_inserter (nhs),
                    [&] (const auto &nh) {
                      return isNextHopEligible (ingress.face, interest, nh, pitEntry);
                    });

      // Get target
      auto target = getTarget (plane, pindex, plane_this, pindex_this);

      // Send to target
      switch (target)
        {
        case SEND_TO_GROUND:
          it = std::find_if (nhs.begin (), nhs.end (), [&] (const fib::NextHop &nexthop) {
            auto sataddress = getSatAddress (nexthop);
            return sataddress.getOrbitalPlane () == plane_this &&
                   sataddress.getPlaneIndex () == pindex_this;
          });
          break;
        case NEXT_PLANE:
          it = std::find_if (nhs.begin (), nhs.end (), [&] (const fib::NextHop &nexthop) {
            auto sataddress = getSatAddress (nexthop);
            return sataddress.getOrbitalPlane () == ((plane_this + 1) % m_nPlanes);
          });
          break;
        case PREVIOUS_PLANE:
          it = std::find_if (nhs.begin (), nhs.end (), [&] (const fib::NextHop &nexthop) {
            auto sataddress = getSatAddress (nexthop);
            return sataddress.getOrbitalPlane () == ((m_nPlanes + plane_this - 1) % m_nPlanes);
          });
          break;
        case NEXT_SAT:
          it = std::find_if (nhs.begin (), nhs.end (), [&] (const fib::NextHop &nexthop) {
            auto sataddress = getSatAddress (nexthop);
            return sataddress.getOrbitalPlane () == plane_this &&
                   sataddress.getPlaneIndex () == ((pindex_this + 1) % m_planeSize);
          });
          break;
        case PREVIOUS_SAT:
          it = std::find_if (nhs.begin (), nhs.end (), [&] (const fib::NextHop &nexthop) {
            auto sataddress = getSatAddress (nexthop);
            return sataddress.getOrbitalPlane () == plane_this &&
                   sataddress.getPlaneIndex () == ((m_planeSize + pindex_this - 1) % m_planeSize);
          });
          break;
        }
      if (it == nhs.end ())
        {
          NFD_LOG_DEBUG (interest << " from=" << ingress << " noNextHop");

          lp::NackHeader nackHeader;
          nackHeader.setReason (lp::NackReason::NO_ROUTE);
          this->sendNack (pitEntry, ingress, nackHeader);

          this->rejectPendingInterest (pitEntry);
          return;
        }
      auto egress = FaceEndpoint (it->getFace (), 0);
      NFD_LOG_DEBUG ("GEOCAST " << interest << " from=" << ingress << " newPitEntry-to=" << egress);
      this->sendInterest (pitEntry, egress, interest);
      return;
    }

  if (suppression == RetxSuppressionResult::NEW)
    {
      // forward to nexthop with lowest cost except downstream
      it = std::find_if (nexthops.begin (), nexthops.end (), [&] (const auto &nexthop) {
        return isNextHopEligible (ingress.face, interest, nexthop, pitEntry);
      });

      if (it == nexthops.end ())
        {
          NFD_LOG_DEBUG (interest << " from=" << ingress << " noNextHop");

          lp::NackHeader nackHeader;
          nackHeader.setReason (lp::NackReason::NO_ROUTE);
          this->sendNack (pitEntry, ingress, nackHeader);

          this->rejectPendingInterest (pitEntry);
          return;
        }
      auto egress = FaceEndpoint (it->getFace (), 0);
      NFD_LOG_DEBUG (interest << " from=" << ingress << " newPitEntry-to=" << egress);
      this->sendInterest (pitEntry, egress, interest);
      return;
    }

  // find an unused upstream with lowest cost except downstream
  it = std::find_if (nexthops.begin (), nexthops.end (), [&] (const auto &nexthop) {
    return isNextHopEligible (ingress.face, interest, nexthop, pitEntry, true,
                              time::steady_clock::now ());
  });

  if (it != nexthops.end ())
    {
      auto egress = FaceEndpoint (it->getFace (), 0);
      this->sendInterest (pitEntry, egress, interest);
      NFD_LOG_DEBUG (interest << " from=" << ingress << " retransmit-unused-to=" << egress);
      return;
    }

  // find an eligible upstream that is used earliest
  it = findEligibleNextHopWithEarliestOutRecord (ingress.face, interest, nexthops, pitEntry);
  if (it == nexthops.end ())
    {
      NFD_LOG_DEBUG (interest << " from=" << ingress << " retransmitNoNextHop");
    }
  else
    {
      auto egress = FaceEndpoint (it->getFace (), 0);
      this->sendInterest (pitEntry, egress, interest);
      NFD_LOG_DEBUG (interest << " from=" << ingress << " retransmit-retry-to=" << egress);
    }
}

void
GeoTagStrategy::afterReceiveNack (const FaceEndpoint &ingress, const lp::Nack &nack,
                                  const shared_ptr<pit::Entry> &pitEntry)
{
  this->processNack (ingress.face, nack, pitEntry);
}

ns3::Ptr<ns3::NetDevice>
GeoTagStrategy::getNetDevice (face::Face &face)
{
  auto transport = face.getTransport ();
  auto ndtransport = dynamic_cast<ns3::ndn::NetDeviceTransport *> (transport);
  if (ndtransport != nullptr)
    {
      return ndtransport->GetNetDevice ();
    }
  // Ground nodes' faces have no NetDeviceTransport
  else
    return nullptr;
}

ns3::Ptr<ns3::NetDevice>
GeoTagStrategy::getRemoteNetDevice (face::Face &face)
{
  auto netDevice = getNetDevice (face)->GetObject<ns3::icarus::SatNetDevice> ();
  if (netDevice != nullptr)
    {
      auto channel = netDevice->GetChannel ();
      if (channel->GetDevice (0) == netDevice)
        {
          return channel->GetDevice (1);
        }
      else
        {
          return channel->GetDevice (0);
        }
    }
  else
    {
      // For Sat2GroundNetDevice, return its own netdevice
      return getNetDevice (face);
    }
}

ns3::icarus::SatAddress
GeoTagStrategy::getSatAddress (const fib::NextHop &nexthop)
{
  auto remote_netdev = getRemoteNetDevice (nexthop.getFace ());
  auto remote_node = remote_netdev->GetNode ();
  auto remote_sat2groundnd = remote_node->GetDevice (0);
  return ns3::icarus::SatAddress::ConvertFrom (remote_sat2groundnd->GetAddress ());
}

GeoTagStrategy::Target
GeoTagStrategy::getTarget (uint16_t plane, uint16_t pindex, uint16_t this_plane,
                           uint16_t this_pindex)
{
  if (plane == this_plane)
    {
      if (pindex == this_pindex)
        return SEND_TO_GROUND;
      else
        {
          auto difPindex = pindex - this_pindex;
          if (difPindex > 0)
            {
              if (difPindex <= m_planeSize / 2)
                return NEXT_SAT;
              else
                return PREVIOUS_SAT;
            }
          else
            {
              if (difPindex >= -m_planeSize / 2)
                return PREVIOUS_SAT;
              else
                return NEXT_SAT;
            }
        }
    }
  else
    {
      auto difPlane = plane - this_plane;
      if (difPlane > 0)
        {
          if (difPlane <= m_nPlanes / 2)
            return NEXT_PLANE;
          else
            return PREVIOUS_PLANE;
        }
      else
        {
          if (difPlane >= -m_nPlanes / 2)
            return PREVIOUS_PLANE;
          else
            return NEXT_PLANE;
        }
    }
}

} // namespace fw
} // namespace nfd
