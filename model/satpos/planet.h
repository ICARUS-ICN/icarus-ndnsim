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

#ifndef PLANET_H
#define PLANET_H

#include <boost/units/dimension.hpp>
#include <boost/units/systems/si/angular_velocity.hpp>
#include <boost/units/systems/si/io.hpp>
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/mass.hpp>

namespace satpos {

namespace planet {
using namespace boost::units;
using namespace boost::units::si;

class Planet
{
  const quantity<mass> m;
  const quantity<length> eq_radius;
  const quantity<angular_velocity> rotation_rate;

public:
  constexpr Planet (const quantity<mass> &m, const quantity<length> &eq_radius,
                    const quantity<angular_velocity> &rotation_rate)
      : m (m), eq_radius (eq_radius), rotation_rate (rotation_rate)
  {
  }

  constexpr auto
  getRadius () const -> quantity<length>
  {
    return eq_radius;
  }
  constexpr auto
  getMass () const -> quantity<mass>
  {
    return m;
  }
  constexpr auto
  getRotationRate () const -> quantity<angular_velocity>
  {
    return rotation_rate;
  }
};

namespace constants {
extern const Planet Earth;
} // namespace constants

} // namespace planet

} // namespace satpos

#endif // PLANET_H