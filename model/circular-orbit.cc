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
#include "circular-orbit.h"

#include "satpos/planet.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/mobility-model.h"
#include "ns3/geographic-positions.h"

#include <boost/math/constants/constants.hpp>
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/angle/degrees.hpp>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CircularOrbitMobilityModel");

using namespace satpos::planet;
using satpos::planet::constants::Earth;

NS_OBJECT_ENSURE_REGISTERED (CircularOrbitMobilityModel);

TypeId
CircularOrbitMobilityModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CircularOrbitMobilityModel")
                          .AddConstructor<CircularOrbitMobilityModel> ()
                          .SetParent<MobilityModel> ()
                          .SetGroupName ("Mobility");

  return tid;
}

constexpr auto G = 6.67430e-11 * newton * square_meter / pow<2> (kilogram);

namespace {
using namespace boost::units;
using namespace boost::units::si;

constexpr quantity<angular_velocity>
getN (CircularOrbitMobilityModel::meters radius, const Planet &planet)
{
  return radians * root<2> (G * planet.getMass () / pow<3> (radius));
}
} // namespace

CircularOrbitMobilityModel::CircularOrbitMobilityModel () : MobilityModel ()
{
}

void
CircularOrbitMobilityModel::LaunchSat (radians inclination, radians ascending_node, meters altitude,
                                       radians phase)
{
  this->inclination = inclination;
  this->ascending_node = ascending_node;
  this->radius = altitude + Earth.getRadius ();
  this->phase = phase;
}

CircularOrbitMobilityModel::~CircularOrbitMobilityModel ()
{
}

void
CircularOrbitMobilityModel::DoSetPosition (const Vector &position)
{
  NS_ABORT_MSG (
      "It is not supported to directly set the position in the CircularOrbitMobilityModel");
}

Vector
CircularOrbitMobilityModel::DoGetVelocity () const
{
  NS_LOG_WARN ("We do not support reporting the proper velocity");

  return Vector ();
}

Vector
CircularOrbitMobilityModel::getRawPosition () const
{
  using namespace boost::math::double_constants;
  using namespace boost::units;
  using namespace boost::units::si;

  const quantity<si::time> now{Simulator::Now ().GetSeconds () * seconds};

  const radians E{getN (radius, Earth) * now + phase};
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
  std::tie (x, y, z) = std::make_tuple (radius * cos_theta, radius * sin_theta, 0.0 * meter);
  // Second step: Inclination correction
  std::tie (x, y, z) = std::make_tuple (x, y * cos (inclination) - z * sin (inclination),
                                        y * sin (inclination) + z * cos (inclination));

  // Third step: Ascending node correction
  std::tie (x, y, z) = std::make_tuple (x * cos (ascending_node) - y * sin (ascending_node),
                                        x * sin (ascending_node) + y * cos (ascending_node), z);

  return Vector (x.value (), y.value (), z.value ());
}

Vector
CircularOrbitMobilityModel::DoGetPosition () const
{
  Vector rawPosition{getRawPosition ()};
  const auto radius{rawPosition.GetLength ()};
  const auto latitude{radian * asin (rawPosition.z / radius)};
  const auto prime_meridian_ascension{0.0 * radian + second * ns3::Simulator::Now ().GetSeconds () *
                                                         Earth.getRotationRate ()};
  const auto sat_ascension{radian * atan2 (rawPosition.y, rawPosition.x)};

  return GeographicPositions::GeographicToCartesianCoordinates (
      quantity<degree::plane_angle> (latitude).value (),
      quantity<degree::plane_angle> (sat_ascension - prime_meridian_ascension).value (),
      radius - Earth.getRadius ().value (), GeographicPositions::SPHERE);
}

} // namespace ns3
