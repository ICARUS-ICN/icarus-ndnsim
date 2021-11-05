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

#include "constellation-helper.h"

#include "ns3/abort.h"
#include "ns3/constellation.h"
#include "ns3/icarus-helper.h"

#include "model/ndn-l3-protocol.hpp"
#include "ns3/assert.h"
#include "ns3/circular-orbit.h"
#include "ns3/core-module.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include "ns3/node-container.h"
#include "ns3/ptr.h"
#include "ns3/sat-address.h"

#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/io.hpp>

NS_LOG_COMPONENT_DEFINE ("icarus.ConstellationHelper");

namespace ns3 {
namespace icarus {

using boost::units::quantity;
using boost::units::degree::degree;
using boost::units::si::length;
using boost::units::si::plane_angle;

ConstellationHelper::ConstellationHelper (quantity<length> altitude,
                                          quantity<plane_angle> inclination, std::size_t n_planes,
                                          std::size_t n_satellites_per_plane, std::size_t n_phases)
    : m_constellation (Create<Constellation> (n_planes, n_satellites_per_plane)),
      m_offsetIncrement (n_phases * 360.0 * degree / (1.0 * n_satellites_per_plane * n_planes)),
      m_altitude (altitude),
      m_inclination (inclination),
      m_ascendingNode (0 * degree),
      m_phase (0 * degree),
      m_offset (0 * degree),
      m_planeIndex (0),
      m_orbitIndex (0)
{
  NS_LOG_FUNCTION (this << altitude << inclination << n_planes << n_satellites_per_plane
                        << n_phases);

  m_circularOrbitFactory.SetTypeId ("ns3::icarus::CircularOrbitMobilityModel");
}

Ptr<Constellation>
ConstellationHelper::GetConstellation () const
{
  NS_LOG_FUNCTION (this);

  return m_constellation;
}

SatAddress
ConstellationHelper::LaunchSatellite (Ptr<Node> satellite)
{
  NS_LOG_FUNCTION (this << &satellite);

  NS_ABORT_MSG_IF (m_planeIndex >= m_constellation->GetNPlanes () && m_orbitIndex > 0,
                   "All satellites have already been created in this constellation");
  NS_ASSERT_MSG (m_phase < 360 * degree, "Phase angle should be < 360º: " << m_phase);
  NS_ASSERT_MSG (m_ascendingNode < 360 * degree,
                 "Ascending node should be < 360º: " << m_ascendingNode);

  auto orbit = m_circularOrbitFactory.Create<CircularOrbitMobilityModel> ();
  orbit->LaunchSat (quantity<plane_angle> (m_inclination), quantity<plane_angle> (m_ascendingNode),
                    m_altitude, quantity<plane_angle> (m_phase + m_offset));

  satellite->AggregateObject (orbit);
  const auto address = m_constellation->AddSatellite (m_planeIndex, m_orbitIndex, satellite);

  m_orbitIndex += 1;
  m_phase += 360 / static_cast<double> (m_constellation->GetPlaneSize ()) * degree;
  if (m_orbitIndex == m_constellation->GetPlaneSize ())
    {
      m_orbitIndex = 0;
      m_phase = 0 * degree;
      m_planeIndex += 1;
      m_ascendingNode += 360 / static_cast<double> (m_constellation->GetNPlanes ()) * degree;
      m_offset += m_offsetIncrement;
    }

  return address;
}

} // namespace icarus
} // namespace ns3
