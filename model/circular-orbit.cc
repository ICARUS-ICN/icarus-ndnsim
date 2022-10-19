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

#include "ns3/assert.h"
#include "orbit/circular-orbit-impl.h"
#include "orbit/satpos/planet.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/mobility-model.h"
#include "ns3/geographic-positions.h"

#include <boost/optional/optional.hpp>
#include <boost/units/quantity.hpp>
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/si/time.hpp>

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

namespace {

constexpr quantity<length> EARTH_RADIUS = Earth.getRadius ();
constexpr quantity<length> EARTH_SEMIMAJOR_AXIS = 6378137 * meters;
constexpr double EARTH_GRS80_ECCENTRICITY = 0.0818191910428158;
constexpr double EARTH_WGS84_ECCENTRICITY = 0.0818191908426215;
// ndnSIM-29 version of ns-3 lacks this method. Copy it here.
std::pair<quantity<plane_angle>, quantity<plane_angle>>
CartesianToGeographicCoordinates (Vector pos, GeographicPositions::EarthSpheroidType sphType)
{
  using boost::units::degree::degrees;

  quantity<length> a; // semi-major axis of earth
  double e; // first eccentricity of earth
  if (sphType == GeographicPositions::SPHERE)
    {
      a = EARTH_RADIUS;
      e = 0;
    }
  else if (sphType == GeographicPositions::GRS80)
    {
      a = EARTH_SEMIMAJOR_AXIS;
      e = EARTH_GRS80_ECCENTRICITY;
    }
  else // if sphType == WGS84
    {
      a = EARTH_SEMIMAJOR_AXIS;
      e = EARTH_WGS84_ECCENTRICITY;
    }

  quantity<plane_angle> longitude, latitude, tmp_latitude;
  longitude = atan2 (pos.y * meters, pos.x * meters); // longitude (rad), in +/- pi

  double e2 = e * e;
  // sqrt (pos.x^2 + pos.y^2)
  quantity<length> p = CalculateDistance (pos, {0, 0, pos.z}) * meters;
  latitude = atan2 (pos.z * meters, p * (1 - e2)); // init latitude (rad), in +/- pi

  do
    {
      tmp_latitude = latitude;
      quantity<length> N = a / sqrt (1 - e2 * sin (tmp_latitude) * sin (tmp_latitude));
      quantity<length> v = p / cos (tmp_latitude);
      // quantity<length> altitude = v - N; // altitude
      latitude = atan2 (pos.z * meters, p * (1 - e2 * N / v));
  }
  // 1 m difference is approx 1 / 30 arc seconds = 9.26e-6 deg
  while (abs (latitude - tmp_latitude) > quantity<plane_angle> (9.26e-6 * degree::degrees));

  quantity<degree::plane_angle> longitude_deg = (quantity<degree::plane_angle> (longitude));
  quantity<degree::plane_angle> latitude_deg = (quantity<degree::plane_angle> (latitude));

  // canonicalize (latitude) x in [-90, 90] and (longitude) y in [-180, 180)
  if (latitude_deg > 90.0 * degrees)
    {
      latitude_deg = 180 * degrees - latitude_deg;
      longitude_deg += longitude_deg < 0 * degrees ? 180 * degrees : -180 * degrees;
    }
  else if (latitude_deg < -90.0 * degrees)
    {
      latitude_deg = -180 * degrees - latitude_deg;
      longitude_deg += longitude_deg < 0 * degrees ? 180 * degrees : -180 * degrees;
    }
  if (longitude_deg == 180.0 * degrees)
    longitude_deg = -180 * degrees;

  // make sure lat/lon in the right range to double check canonicalization
  // and conversion routine
  NS_ASSERT_MSG (-180.0 * degrees <= longitude_deg, "Conversion error: longitude too negative");
  NS_ASSERT_MSG (180.0 * degrees > longitude_deg, "Conversion error: longitude too positive");
  NS_ASSERT_MSG (-90.0 * degrees <= latitude_deg, "Conversion error: latitude too negative");
  NS_ASSERT_MSG (90.0 * degrees >= latitude_deg, "Conversion error: latitude too positive");

  return std::make_pair (quantity<plane_angle> (latitude_deg),
                         quantity<plane_angle> (longitude_deg));
}
} // namespace

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
CircularOrbitMobilityModel::getGroundDistanceAtElevation (radians elevation,
                                                          meters ground_radius) const noexcept
{
  NS_LOG_FUNCTION (this << elevation.value ());
  NS_ABORT_IF (sat == nullptr);

  return sat->getGroundDistanceAtElevation (elevation, ground_radius).value ();
}

ns3::Time
CircularOrbitMobilityModel::getNextTimeAtDistance (meters distance, Ptr<Node> ground,
                                                   boost::optional<Time> t0) const noexcept
{
  NS_LOG_FUNCTION (this << distance.value ());
  NS_ABORT_IF (sat == nullptr);

  const Ptr<MobilityModel> groundmodel = ground->GetObject<ConstantPositionMobilityModel> ();
  NS_ASSERT_MSG (groundmodel, "We only support static ground nodes!");

  const quantity<si::time> init_time = (t0 ? *t0 : Simulator::Now ()).GetSeconds () * si::seconds;

  const Vector pos (groundmodel->GetPosition ());
  const auto angular_pos = CartesianToGeographicCoordinates (pos, GeographicPositions::WGS84);

  return Seconds (sat->getNextTimeAtDistance (init_time, distance, angular_pos.first,
                                              angular_pos.second,
                                              pos.GetLength () * boost::units::si::meters)
                      .value ());
}

ns3::Time
CircularOrbitMobilityModel::getNextTimeAtElevation (radians elevation, Ptr<Node> ground,
                                                    boost::optional<Time> t0) const noexcept
{
  NS_LOG_FUNCTION (this << elevation.value ());
  NS_ABORT_IF (sat == nullptr);

  const Ptr<MobilityModel> groundmodel = ground->GetObject<ConstantPositionMobilityModel> ();
  NS_ASSERT_MSG (groundmodel, "We only support static ground nodes!");

  const auto ground_radius = groundmodel->GetPosition ().GetLength () * boost::units::si::meters;

  return getNextTimeAtDistance (sat->getGroundDistanceAtElevation (elevation, ground_radius),
                                ground, t0);
}

} // namespace icarus
} // namespace ns3
