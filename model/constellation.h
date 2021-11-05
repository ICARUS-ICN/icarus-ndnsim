/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Universidade de Vigo
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
 * Author: Miguel Rodríguez Pérez <miguel@det.uvigo.gal>
 */
#ifndef CONSTELLATION_H
#define CONSTELLATION_H

#include "ns3/node-container.h"
#include "ns3/ptr.h"
#include "ns3/simple-ref-count.h"
#include "ns3/vector.h"

#include <vector>

namespace ns3 {

class Node;

namespace icarus {

class SatAddress;

class Constellation : public SimpleRefCount<Constellation>
{
public:
  Constellation (std::size_t n_planes, std::size_t plane_size);
  Constellation (const Constellation &) = delete;

  SatAddress AddSatellite (std::size_t plane, std::size_t plane_order, Ptr<Node> satellite);
  Ptr<Node> GetClosest (Vector3D cartesianCoordinates) const;

  std::size_t GetNPlanes () const;
  std::size_t GetPlaneSize () const;
  std::size_t GetConstellationId () const;
  Ptr<Node> GetSatellite (const SatAddress &address) const;
  Ptr<Node> GetSatellite (std::size_t plane, std::size_t index) const;

  NodeContainer CreateNodeContainer () const;

private:
  typedef std::vector<Ptr<Node>> plane;

  static std::size_t constellationCounter;

  std::size_t m_constellationId;
  std::size_t m_nPlanes, m_planeSize;
  std::vector<plane> m_planes;
};
} // namespace icarus
} // namespace ns3

#endif
