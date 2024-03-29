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

#ifndef GROUND_SAT_SUCCESS_MODEL_H
#define GROUND_SAT_SUCCESS_MODEL_H

#include "ns3/ptr.h"
#include "ns3/type-id.h"
#include "ns3/object.h"

namespace ns3 {

class Node;
class Packet;

namespace icarus {

class GroundSatSuccessModel : public Object
{
public:
  static TypeId GetTypeId (void);
  virtual ~GroundSatSuccessModel ();

  virtual bool TramsmitSuccess (const Ptr<Node> &srcNode, const Ptr<Node> &dstNode,
                                const Ptr<Packet> &packet) const = 0;
};

} // namespace icarus
} // namespace ns3

#endif
