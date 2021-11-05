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

#include "constellation.h"
#include "circular-orbit.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/node-container.h"
#include "ns3/node.h"

#include <limits>

namespace ns3 {
namespace icarus {

NS_LOG_COMPONENT_DEFINE ("icarus.Constellation");

std::size_t Constellation::constellationCounter = 0;

namespace {
auto
getSqDistance (const Vector &a, const Vector &b)
{
  auto res = b - a;

  return res.x * res.x + res.y * res.y + res.z * res.z;
}
} // namespace

Constellation::Constellation (std::size_t n_planes, std::size_t plane_size)
    : m_constellationId (constellationCounter++),
      m_nPlanes (n_planes),
      m_planeSize (plane_size),
      m_planes (n_planes)
{
  NS_LOG_FUNCTION (this << n_planes << plane_size);

  for (plane &p : m_planes)
    {
      p.resize (plane_size);
    }
}

void
Constellation::AddSatellite (std::size_t plane, std::size_t plane_order, Ptr<Node> satellite)
{
  NS_LOG_FUNCTION (this << plane << plane_order << satellite);

  NS_ABORT_MSG_IF (m_planes[plane][plane_order] != nullptr,
                   "There can be only on satellite in each orbital location");
  NS_ABORT_MSG_UNLESS (satellite->GetObject<CircularOrbitMobilityModel> () != nullptr,
                       "A satellite must have a CircularOrbitMobilityModel");

  m_planes[plane][plane_order] = satellite;
}

Ptr<Node>
Constellation::GetClosest (Vector3D cartesianCoordinates) const
{
  NS_LOG_FUNCTION (this << cartesianCoordinates);

  Ptr<Node> closest = nullptr;
  Vector bestPos;
  double sq_closest_distance = std::numeric_limits<double>::infinity ();

  NS_LOG_WARN ("FIXME: Replace this with a better algorithm.");
  for (const auto &plane : m_planes)
    {
      for (const auto &sat : plane)
        {
          if (sat == nullptr)
            {
              continue;
            }

          auto sq_distance =
              getSqDistance (sat->GetObject<MobilityModel> ()->GetPosition (), bestPos);
          if (sq_distance <= sq_closest_distance)
            {
              sq_distance = sq_closest_distance;
              bestPos = sat->GetObject<MobilityModel> ()->GetPosition ();
              closest = sat;
            }
        }
    }

  return closest;
}

std::size_t
Constellation::GetNPlanes () const
{
  NS_LOG_FUNCTION (this);

  return m_nPlanes;
}

std::size_t
Constellation::GetPlaneSize () const
{
  NS_LOG_FUNCTION (this);

  return m_planeSize;
}

Ptr<Node>
Constellation::GetSatellite (std::size_t plane, std::size_t index) const
{
  NS_LOG_FUNCTION (this << plane << index);

  NS_ASSERT_MSG (plane <= GetNPlanes (), "Plane " << plane << " is outside range");
  NS_ASSERT_MSG (index <= GetPlaneSize (), "Index " << plane << " is outside range");

  return m_planes[plane][index];
}

NodeContainer
Constellation::CreateNodeContainer () const
{
  NS_LOG_FUNCTION (this);
  NodeContainer nodes;

  for (const auto &plane : m_planes)
    {
      for (const auto &sat : plane)
        {
          if (sat != nullptr)
            {
              nodes.Add (sat);
            }
        }
    }

  return nodes;
}

std::size_t
Constellation::GetConstellationId () const
{
  return m_constellationId;
}

} // namespace icarus
} // namespace ns3
