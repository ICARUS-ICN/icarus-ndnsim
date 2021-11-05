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

#include "ns3/constellation.h"
#include "ns3/icarus-helper.h"

#include "model/ndn-l3-protocol.hpp"
#include "ns3/assert.h"
#include "ns3/circular-orbit.h"
#include "ns3/core-module.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include "ns3/node-container.h"
#include "ns3/ptr.h"
#include "ns3/sat-address.h"

#include <boost/units/systems/angle/degrees.hpp>

NS_LOG_COMPONENT_DEFINE ("icarus.ConstellationHelper");

namespace ns3 {
namespace icarus {

using boost::units::quantity;
using boost::units::degree::degree;
using boost::units::si::length;
using boost::units::si::plane_angle;

ConstellationHelper::ConstellationHelper (Ptr<IcarusHelper> creator) : m_icarusHelper (creator)
{
  NS_LOG_FUNCTION (this << creator);

  m_circularOrbitFactory.SetTypeId ("ns3::icarus::CircularOrbitMobilityModel");
}

Ptr<Constellation>
ConstellationHelper::CreateConstellation (quantity<length> altitude,
                                          quantity<plane_angle> inclination, unsigned n_planes,
                                          unsigned n_satellites_per_plane, unsigned n_phases)
{
  NS_LOG_FUNCTION (this << altitude.value () << inclination.value () << n_satellites_per_plane
                        << n_planes << n_phases);
  auto constellation{Create<Constellation> (n_planes, n_satellites_per_plane)};

  NodeContainer nodes;
  nodes.Create (n_planes * n_satellites_per_plane);

  std::vector<SatAddress> addresses;
  addresses.reserve (nodes.size ());

  const auto offset_increment{n_phases * 360.0 * degree /
                              (1.0 * n_satellites_per_plane * n_planes)};

  auto satIterator = nodes.begin ();
  auto plane_index = 0;
  for (auto ascending_node{0 * degree}, offset = 0 * degree; ascending_node < 360 * degree;
       ascending_node += 360 / static_cast<double> (n_planes) * degree, offset += offset_increment,
                                        plane_index++)
    {
      auto orbit_index = 0;
      for (auto phase{0 * degree}; phase < 360 * degree;
           phase += 360 / static_cast<double> (n_satellites_per_plane) * degree, satIterator++,
           orbit_index++)
        {
          NS_ASSERT (satIterator != nodes.end ());
          NS_ASSERT (*satIterator != nullptr);

          auto orbit = m_circularOrbitFactory.Create<CircularOrbitMobilityModel> ();
          orbit->LaunchSat (inclination, quantity<plane_angle> (ascending_node), altitude,
                            quantity<plane_angle> (phase + offset));

          (*satIterator)->AggregateObject (orbit);
          constellation->AddSatellite (plane_index, orbit_index, *satIterator);
          addresses.push_back (
              SatAddress (constellation->GetConstellationId (), plane_index, orbit_index));
        }
    }

  // Now that they have mobility, we can install Icarus net devices
  m_icarusHelper->Install (nodes, addresses);

  return constellation;
}

} // namespace icarus
} // namespace ns3
