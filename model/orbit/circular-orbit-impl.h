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
#ifndef CIRCULAR_ORBIT_IMPL_H
#define CIRCULAR_ORBIT_IMPL_H

#include <boost/units/quantity.hpp>
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/si/time.hpp>

namespace ns3 {
namespace icarus {

class CircularOrbitMobilityModelImpl
{
public:
  typedef boost::units::quantity<boost::units::si::plane_angle> radians;
  typedef boost::units::quantity<boost::units::si::length> meters;
  typedef boost::units::quantity<boost::units::si::time> time;

  CircularOrbitMobilityModelImpl (radians inclination, radians ascending_node, meters radius,
                                  radians phase) noexcept;

  std::tuple<meters, meters, meters>
  getCartesianPositionRightAscensionDeclination (time t) const noexcept;

  meters getRadius () const noexcept;

  meters getGroundAltitude () const noexcept;

  meters getGroundDistanceAtElevation (radians elevation) const noexcept;

private:
  const radians inclination;
  const radians ascending_node;
  const meters radius;
  const radians phase;
};

} // namespace icarus
} // namespace ns3

#endif /* CIRCULAR_ORBIT_IMPL_H */
