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

#ifndef GROUND_SAT_SUCCESS_ELEVATION_H
#define GROUND_SAT_SUCCESS_ELEVATION_H

#include "ground-sat-success-model.h"

#include <boost/units/quantity.hpp>
#include <boost/units/systems/si/plane_angle.hpp>

namespace ns3 {
namespace icarus {

class GroundSatSuccessElevation : public GroundSatSuccessModel
{
public:
  static TypeId GetTypeId (void);
  virtual ~GroundSatSuccessElevation () = default;

  virtual bool TramsmitSuccess (const Ptr<Node> &srcNode, const Ptr<Node> &dstNode,
                                const Ptr<Packet> &packet) const noexcept override;

  void SetMinimumElevationDegrees (double minElevation) noexcept;
  double GetMinimumElevationDegrees () const noexcept;

private:
  boost::units::quantity<boost::units::si::plane_angle> m_minimumElevation;
};

} // namespace icarus
} // namespace ns3

#endif
