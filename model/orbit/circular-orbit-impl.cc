/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021–22 Universidade de Vigo
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
#include "circular-orbit-impl.h"

#include "satpos/planet.h"

#include <ns3/simulator.h>

#include <boost/units/quantity.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/units/systems/si/angular_velocity.hpp>
#include <boost/units/systems/si/area.hpp>
#include <boost/units/systems/si/force.hpp>
#include <boost/units/systems/si/mass.hpp>
#include <boost/units/cmath.hpp>
#include <boost/units/pow.hpp>

namespace {

using namespace boost::units;
using namespace boost::units::si;

constexpr auto G = 6.67430e-11 * newton * square_meter / pow<2> (kilogram);

constexpr quantity<angular_velocity>
getN (quantity<length> radius, const icarus::satpos::planet::Planet &planet)
{
  return radians * root<2> (G * planet.getMass () / pow<3> (radius));
}
} // namespace

using icarus::satpos::planet::constants::Earth;

namespace ns3 {
namespace icarus {

CircularOrbitMobilityModelImpl::CircularOrbitMobilityModelImpl (radians inclination,
                                                                radians ascending_node,
                                                                meters radius,
                                                                radians phase) noexcept
    : inclination (inclination), ascending_node (ascending_node), radius (radius), phase (phase)
{
}

std::tuple<CircularOrbitMobilityModelImpl::meters, CircularOrbitMobilityModelImpl::meters,
           CircularOrbitMobilityModelImpl::meters>
CircularOrbitMobilityModelImpl::getCartesianPositionRightAscensionDeclination (
    quantity<si::time> t) const noexcept
{
  using namespace boost::math::double_constants;
  using namespace boost::units;
  using namespace boost::units::si;

  const radians E{getN (radius, Earth) * t + phase};
  const double cos_theta{(cos (E) - 0.0) / (1 - 0.0 * cos (E))}; // Eccentricity is 0
  auto theta{2.0 * atan2 (sin (E / 2.0), cos (E / 2.0))};

  if (E > theta)
    {
      double n{round ((E - theta) / (two_pi * si::radians))};
      theta += n * two_pi * si::radians;
    }
  else
    {
      double n{round ((theta - E) / (two_pi * si::radians))};
      theta -= n * two_pi * si::radians;
    }
  NS_ASSERT (abs (E - theta) < pi * si::radians);

  const double sin_theta{sin (theta)};

  // First step: Perigee angle correction is not needed for circular orbit
  // x = x * cos (0) - y * sin (0);
  // y = x * sin (0) + y * cos (0);
  // z = z;

  meters x, y, z;
  x = radius * cos_theta;
  y = radius * sin_theta;
  z = 0.0 * meter;

  // Second step: Inclination correction
  std::tie (x, y, z) = std::make_tuple (x, //
                                        y * cos (inclination) - z * sin (inclination), //
                                        y * sin (inclination) + z * cos (inclination));

  // Third step: Ascending node correction
  std::tie (x, y, z) = std::make_tuple (x * cos (ascending_node) - y * sin (ascending_node),
                                        x * sin (ascending_node) + y * cos (ascending_node),
                                        z); // z

  return std::make_tuple (x, y, z);
}

CircularOrbitMobilityModelImpl::meters
CircularOrbitMobilityModelImpl::getRadius () const noexcept
{
  return radius;
}

CircularOrbitMobilityModelImpl::meters
CircularOrbitMobilityModelImpl::getGroundAltitude () const noexcept
{
  return getRadius () - Earth.getRadius ();
}

CircularOrbitMobilityModelImpl::meters
CircularOrbitMobilityModelImpl::getGroundDistanceAtElevation (
    CircularOrbitMobilityModelImpl::radians elevation) const noexcept
{
  const quantity<length> alt_ground = getGroundAltitude ();
  const quantity<length> earth_radius = Earth.getRadius ();

  return root<2> (2.0 * pow<2> (earth_radius * sin (elevation)) -
                  2.0 * earth_radius * sin (elevation) *
                      root<2> (pow<2> (earth_radius * sin (elevation)) +
                               2.0 * earth_radius * alt_ground + pow<2> (alt_ground)) +
                  2.0 * earth_radius * alt_ground + pow<2> (alt_ground));
}

} // namespace icarus
} // namespace ns3