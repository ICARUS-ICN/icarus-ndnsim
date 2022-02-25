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

#ifndef NFD_DAEMON_FW_GEO_TAG_STRATEGY_HPP
#define NFD_DAEMON_FW_GEO_TAG_STRATEGY_HPP

#include "ns3/ndnSIM/NFD/daemon/face/face-common.hpp"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/vector.h"
#include "ns3/sat-address.h"
#include "ns3/ndnSIM/NFD/daemon/fw/process-nack-traits.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/retx-suppression-exponential.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/strategy.hpp"
#include <boost/tuple/detail/tuple_basic.hpp>
#include <cstddef>
#include <cstdint>
#include <tuple>

namespace nfd {
namespace fw {

/** \brief Geo-Tag strategy
 *
 *  This strategy forwards Interests with GeoTag to the nearest nexthop to the
 * satellite pointed by the GeoTag, and Interests without GeoTag are forwarded
 * like in Best Route Strategy 2.
 *
 *  Best Route Strategy 2 forwards a new Interest to the lowest-cost nexthop (except
 * downstream). After that, if consumer retransmits the Interest (and is not
 * suppressed according to exponential backoff algorithm), the strategy forwards
 * the Interest again to the lowest-cost nexthop (except downstream) that is not
 * previously used. If all nexthops have been used, the strategy starts over
 * with the first nexthop.
 *
 *  This strategy returns Nack to all downstreams with reason NoRoute
 *  if there is no usable nexthop, which may be caused by:
 *  (a) the FIB entry contains no nexthop;
 *  (b) the FIB nexthop happens to be the sole downstream;
 *  (c) the FIB nexthops violate scope.
 *
 *  This strategy returns Nack to all downstreams if all upstreams have returned
 * Nacks. The reason of the sent Nack equals the least severe reason among
 * received Nacks.
 *
 *  \note This strategy is not EndpointId-aware.
 */
class GeoTagStrategy : public Strategy, public ProcessNackTraits<GeoTagStrategy>
{
private:
  enum Target { SEND_TO_GROUND, NEXT_PLANE, PREVIOUS_PLANE, NEXT_SAT, PREVIOUS_SAT };
  std::size_t m_nPlanes, m_planeSize;

public:
  explicit GeoTagStrategy (Forwarder &forwarder, const Name &name = getStrategyName ());

  static const Name &getStrategyName ();

  void afterReceiveInterest (const FaceEndpoint &ingress, const Interest &interest,
                             const shared_ptr<pit::Entry> &pitEntry) override;

  void afterReceiveNack (const FaceEndpoint &ingress, const lp::Nack &nack,
                         const shared_ptr<pit::Entry> &pitEntry) override;

  ns3::Ptr<ns3::NetDevice> getNetDevice (face::Face &face);

  ns3::Ptr<ns3::NetDevice> getRemoteNetDevice (face::Face &face);

  ns3::icarus::SatAddress getSatAddress (const fib::NextHop &nexthop);

  Target getTarget (uint16_t plane, uint16_t pindex, uint16_t this_plane, uint16_t this_pindex);

  PUBLIC_WITH_TESTS_ELSE_PRIVATE : static const time::milliseconds RETX_SUPPRESSION_INITIAL;
  static const time::milliseconds RETX_SUPPRESSION_MAX;
  RetxSuppressionExponential m_retxSuppression;

  friend ProcessNackTraits<GeoTagStrategy>;
};

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_GEO_TAG_STRATEGY_HPP
