/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *
 * Copyright (c) 2021–2022 Universidade de Vigo
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

#ifndef GROUND_NODE_SAT_TRACKER_PERIODIC_H
#define GROUND_NODE_SAT_TRACKER_PERIODIC_H

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/vector.h"
#include "ground-node-sat-tracker.h"

namespace ns3 {

class MobilityModel;

namespace icarus {

class GroundNodeSatTrackerPeriodic : public GroundNodeSatTracker
{
public:
  static TypeId GetTypeId (void) noexcept;
  virtual ~GroundNodeSatTrackerPeriodic () noexcept = default;

private:
  Time m_interval;

  // Cache these pointers
  mutable Ptr<MobilityModel> m_mobilityModel = nullptr;

  void DoInitialize () override;

  Vector3D GetPosition () const noexcept;

  void PeriodicUpdate () const noexcept;
  void UpdateOnce () const noexcept;
};

} // namespace icarus
} // namespace ns3

#endif
