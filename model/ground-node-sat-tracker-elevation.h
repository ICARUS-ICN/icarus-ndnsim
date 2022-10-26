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
 * Author: Miguel Rodríguez Pérez <miguel@det.uvigo.gal>
 *
 */

#ifndef GROUND_NODE_SAT_TRACKER_ELEVATION_H
#define GROUND_NODE_SAT_TRACKER_ELEVATION_H

#include "ground-node-sat-tracker.h"

#include "ndn-cxx/util/signal/signal.hpp"
#include "ns3/vector.h"
#include "ns3/nstime.h"

#include <boost/units/quantity.hpp>
#include <boost/units/systems/si/plane_angle.hpp>

namespace ns3 {
namespace icarus {

class Constellation;

class GroundNodeSatTrackerElevation : public GroundNodeSatTracker
{
public:
  static TypeId GetTypeId (void) noexcept;

  virtual ~GroundNodeSatTrackerElevation () noexcept = default;

  void setElevation (double min_elevation) noexcept;
  double getElevation () const noexcept;

  ::ndn::util::signal::Signal<GroundNodeSatTrackerElevation,
                              const std::vector<std::tuple<Time, std::size_t, std::size_t>> &>
      satsAvailable;

private:
  boost::units::quantity<boost::units::si::plane_angle> m_elevation;

  void DoInitialize () override;
  void Update () noexcept;

  std::vector<std::tuple<Time, std::size_t, std::size_t>> getVisibleSats () const noexcept;
};

} // namespace icarus
} // namespace ns3

#endif
