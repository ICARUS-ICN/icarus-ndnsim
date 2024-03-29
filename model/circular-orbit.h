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
#ifndef CIRCULAR_ORBIT_H
#define CIRCULAR_ORBIT_H

#include "ns3/mobility-model.h"

#include "ns3/simulator.h"

#include <boost/optional/optional.hpp>
#include <boost/units/cmath.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/si/length.hpp>
#include <memory>

namespace ns3 {

class Node;
namespace icarus {

class CircularOrbitMobilityModelImpl;
/**
 * \ingroup icarus
 *
 * \brief Mobility model for which the current position does follows a circular orbit around Earth.
 */
class CircularOrbitMobilityModel : public MobilityModel
{
public:
  typedef boost::units::quantity<boost::units::si::plane_angle> radians;
  typedef boost::units::quantity<boost::units::si::length> meters;
  /**
   * Register this type with the TypeId system.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  CircularOrbitMobilityModel ();
  virtual ~CircularOrbitMobilityModel ();

  void LaunchSat (radians inclination, radians ascending_node, meters altitude, radians phase);
  // Get the position without planet rotation correction
  Vector getRawPosition () const;
  double getRadius () const noexcept;
  double getGroundDistanceAtElevation (radians elevation, meters ground_radius) const noexcept;
  Time getOrbitalPeriod () const noexcept;
  radians getSatElevation (Vector groundPosition) const noexcept;
  ns3::Time getNextTimeAtDistance (meters distance, Ptr<Node> ground,
                                   boost::optional<ns3::Time> t0 = {}) const noexcept;
  ns3::Time getNextTimeAtElevation (radians elevation, Ptr<Node> ground,
                                    boost::optional<ns3::Time> t0 = {}) const noexcept;
  // These two functions only search for a solution in the near future of t0
  boost::optional<ns3::Time>
  tryGetNextTimeAtDistance (meters distance, Ptr<Node> ground,
                            boost::optional<ns3::Time> t0 = {}) const noexcept;
  boost::optional<ns3::Time>
  tryGetNextTimeAtElevation (radians elevation, Ptr<Node> ground,
                             boost::optional<ns3::Time> t0 = {}) const noexcept;

private:
  virtual Vector DoGetPosition (void) const;
  virtual void DoSetPosition (const Vector &position);
  virtual Vector DoGetVelocity (void) const;

  std::unique_ptr<CircularOrbitMobilityModelImpl> sat;
};
} // namespace icarus
} // namespace ns3

#endif /* CIRCULAR_ORBIT_H */
