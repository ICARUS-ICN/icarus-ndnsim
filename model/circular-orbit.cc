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

#include "orbit/circular-orbit-impl.h"
#include "orbit/satpos/planet.h"
#include "ns3/abort.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/mobility-model.h"
#include "ns3/geographic-positions.h"

#include <boost/units/quantity.hpp>
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/angle/degrees.hpp>

using namespace icarus::satpos::planet;
using icarus::satpos::planet::constants::Earth;

namespace ns3 {
namespace icarus {
NS_LOG_COMPONENT_DEFINE ("icarus.CircularOrbitMobilityModel");

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
CircularOrbitMobilityModel::getRadius () const noexcept
{
  NS_LOG_FUNCTION (this);
  NS_ABORT_IF (sat == nullptr);

  return sat->getRadius ().value ();
}

double
CircularOrbitMobilityModel::getGroundDistanceAtElevation (radians elevation) const noexcept
{
  NS_LOG_FUNCTION (this << elevation.value ());
  NS_ABORT_IF (sat == nullptr);

  return sat->getGroundDistanceAtElevation (elevation).value ();
}

} // namespace icarus
} // namespace ns3
