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

#ifndef GROUND_SAT_SUCCESS_DISTANCE_H
#define GROUND_SAT_SUCCESS_DISTANCE_H

#include "ground-sat-success-model.h"

namespace ns3 {
namespace icarus {

class GroundSatSuccessDistance : public GroundSatSuccessModel
{
public:
  static TypeId GetTypeId (void);
  virtual ~GroundSatSuccessDistance ();

  virtual bool TramsmitSuccess (Ptr<Node> srcNode, Ptr<Node> dstNode,
                                Ptr<Packet> packet) const override;

private:
  static const double DEFAULT_MAX_DISTANCE;

  double m_maxDistance; // In meters
};

} // namespace icarus
} // namespace ns3

#endif
