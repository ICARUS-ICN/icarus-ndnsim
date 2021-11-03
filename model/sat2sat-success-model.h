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
 * Authors: Miguel Rodríguez Pérez <miguel@det.uvigo.gal>
 *          Pablo Iglesias Sanuy <pabliglesias@alumnos.uvigo.es>
 *
 */

#ifndef SAT2SAT_SUCCESS_MODEL_H
#define SAT2SAT_SUCCESS_MODEL_H

#include "ns3/ptr.h"
#include "ns3/type-id.h"
#include "ns3/object.h"

namespace ns3 {

class Node;
class Packet;

namespace icarus {

class Sat2SatSuccessModel : public Object
{
public:
  static TypeId GetTypeId (void);
  Sat2SatSuccessModel ();
  virtual ~Sat2SatSuccessModel ();

  virtual bool TramsmitSuccess (Ptr<Node> srcNode, Ptr<Node> dstNode, Ptr<Packet> packet) const;

  virtual void CalcMaxDistance(double height);

private:
  static const double DEFAULT_MAX_DISTANCE;

  double m_maxDistance; // In meters
};

} // namespace icarus
} // namespace ns3

#endif
