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
#include "circular-orbit.h"

#include "ns3/abort.h"
#include "ns3/log-macros-enabled.h"
#include "satpos/planet.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/mobility-model.h"
#include "ns3/geographic-positions.h"

#include <boost/math/constants/constants.hpp>
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/angle/degrees.hpp>
#include <memory>
#include <tuple>

namespace ns3 {
namespace icarus {
NS_LOG_COMPONENT_DEFINE ("icarus.CircularOrbitMobilityModel");

using namespace ::icarus::satpos::planet;
using constants::Earth;

NS_OBJECT_ENSURE_REGISTERED (CircularOrbitMobilityModel);

TypeId
CircularOrbitMobilityModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::icarus::CircularOrbitMobilityModel")
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

class CircularOrbitMobilityModelImpl
{
public:
  typedef boost::units::quantity<boost::units::si::plane_angle> radians;
  typedef boost::units::quantity<boost::units::si::length> meters;

  CircularOrbitMobilityModelImpl (radians inclination, radians ascending_node, meters radius,
                                  radians phase) noexcept
      : inclination (inclination), ascending_node (ascending_node), radius (radius), phase (phase)
  {
  }

  std::tuple<meters, meters, meters>
  getCartesianPositionRightAscensionDeclination (quantity<si::time> t) const noexcept
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

  meters
  getRadius () const noexcept
  {
    return radius;
  }

private:
  const radians inclination;
  const radians ascending_node;
  const meters radius;
  const radians phase;
};

CircularOrbitMobilityModel::CircularOrbitMobilityModel () : MobilityModel (), sat (nullptr)
{
  NS_LOG_FUNCTION (this);
}

void
CircularOrbitMobilityModel::LaunchSat (radians inclination, radians ascending_node, meters altitude,
                                       radians phase)
{
  NS_LOG_FUNCTION (this << inclination << ascending_node << altitude << phase);

  sat = std::make_unique<CircularOrbitMobilityModelImpl> (inclination, ascending_node,
                                                          altitude + Earth.getRadius (), phase);
}

CircularOrbitMobilityModel::~CircularOrbitMobilityModel ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
CircularOrbitMobilityModel::DoSetPosition (const Vector &position)
{
  NS_LOG_FUNCTION (this << position);
  NS_ABORT_MSG (
      "It is not supported to directly set the position in the CircularOrbitMobilityModel");
}

Vector
CircularOrbitMobilityModel::DoGetVelocity () const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_WARN ("We do not support reporting the proper velocity");

  return Vector ();
}

Vector
CircularOrbitMobilityModel::getRawPosition () const
{
  NS_LOG_FUNCTION (this);
  NS_ABORT_IF (sat == nullptr);

  meters x, y, z;
  std::tie (x, y, z) = sat->getCartesianPositionRightAscensionDeclination (
      Simulator::Now ().GetSeconds () * seconds);

  return Vector (x.value (), y.value (), z.value ());
}

Vector
CircularOrbitMobilityModel::DoGetPosition () const
{
  NS_LOG_FUNCTION (this);

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

double
CircularOrbitMobilityModel::getRadius () const
{
  NS_LOG_FUNCTION (this);
  NS_ABORT_IF (sat == nullptr);

  return sat->getRadius ().value ();
}

} // namespace icarus
} // namespace ns3
