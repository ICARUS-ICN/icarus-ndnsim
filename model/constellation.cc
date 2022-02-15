/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021–2022 Universidade de Vigo
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
#include "ns3/net-device-container.h"
#include "ns3/sat-address.h"
#include "ns3/sat2ground-net-device.h"

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
    : m_constellationId (++constellationCounter),
      m_nPlanes (n_planes),
      m_planeSize (plane_size),
      m_planes (n_planes),
      m_size (0)
{
  NS_LOG_FUNCTION (this << n_planes << plane_size);

  for (plane &p : m_planes)
    {
      p.resize (plane_size);
    }
}

SatAddress
Constellation::AddSatellite (std::size_t plane, std::size_t plane_order,
                             Ptr<Sat2GroundNetDevice> satellite)
{
  NS_LOG_FUNCTION (this << plane << plane_order << satellite);

  NS_ABORT_MSG_IF (m_planes[plane][plane_order] != nullptr,
                   "There can be only on satellite in each orbital location");
  NS_ABORT_MSG_UNLESS (satellite->GetNode ()->GetObject<CircularOrbitMobilityModel> () != nullptr,
                       "A satellite must have a CircularOrbitMobilityModel");

  if (m_planes[plane][plane_order] == nullptr)
    {
      m_size += 1;
    }

  m_planes[plane][plane_order] = satellite;

  return SatAddress (m_constellationId, plane, plane_order);
}

Ptr<Sat2GroundNetDevice>
Constellation::GetClosest (Vector3D cartesianCoordinates) const
{
  NS_LOG_FUNCTION (this << cartesianCoordinates);

  Ptr<Sat2GroundNetDevice> closest = nullptr;
  Vector bestPos;
  auto sq_closest_distance = std::numeric_limits<double>::infinity ();

  NS_LOG_WARN ("FIXME: Replace this with a better algorithm.");
  for (const auto &plane : m_planes)
    {
      for (const auto &sat_device : plane)
        {
          if (sat_device == nullptr)
            {
              continue;
            }

          const auto sq_distance =
              getSqDistance (sat_device->GetNode ()->GetObject<MobilityModel> ()->GetPosition (),
                             cartesianCoordinates);
          if (sq_distance <= sq_closest_distance)
            {
              sq_closest_distance = sq_distance;
              bestPos = sat_device->GetNode ()->GetObject<MobilityModel> ()->GetPosition ();
              closest = sat_device;
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

Ptr<Sat2GroundNetDevice>
Constellation::GetSatellite (const SatAddress &address) const
{
  NS_LOG_FUNCTION (this << address);

  NS_ASSERT_MSG (address.getConstellationId () == GetConstellationId (),
                 "Cannot get an address from constellation " << address.getConstellationId ()
                                                             << " in constellation "
                                                             << GetConstellationId () << ".");

  return GetSatellite (address.getOrbitalPlane (), address.getPlaneIndex ());
}

Ptr<Sat2GroundNetDevice>
Constellation::GetSatellite (std::size_t plane, std::size_t index) const
{
  NS_LOG_FUNCTION (this << plane << index);

  NS_ASSERT_MSG (plane <= GetNPlanes (), "Plane " << plane << " is outside range");
  NS_ASSERT_MSG (index <= GetPlaneSize (), "Index " << plane << " is outside range");

  return m_planes[plane][index];
}

NetDeviceContainer
Constellation::CreateNetDeviceContainer () const
{
  NS_LOG_FUNCTION (this);
  NetDeviceContainer devices;

  for (const auto &plane : m_planes)
    {
      for (const auto &sat : plane)
        {
          if (sat != nullptr)
            {
              devices.Add (sat);
            }
        }
    }

  return devices;
}

std::size_t
Constellation::GetConstellationId () const
{
  return m_constellationId;
}

std::size_t
Constellation::GetSize () const
{
  NS_ASSERT (m_size <= m_nPlanes * m_planeSize);

  return m_size;
}

Ptr<Sat2GroundNetDevice>
Constellation::Get (std::size_t index) const
{
  NS_ABORT_UNLESS (index <= GetSize ());

  const std::size_t plane = index / GetPlaneSize ();
  const std::size_t plane_order = index % GetPlaneSize ();

  return m_planes[plane][plane_order];
}

} // namespace icarus
} // namespace ns3
