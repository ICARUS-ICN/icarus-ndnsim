/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2022 Universidade de Vigo
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

#ifndef DISTANCE_SOLVER_H
#define DISTANCE_SOLVER_H

#include "../circular-orbit-impl.h"

#include <boost/optional/optional.hpp>

namespace ns3 {
namespace icarus {
namespace orbit {

boost::optional<boost::units::quantity<boost::units::si::time>>
findNextCross (boost::units::quantity<boost::units::si::time> now,
               CircularOrbitMobilityModelImpl satellite,
               boost::units::quantity<boost::units::si::length> distance,
               boost::units::quantity<boost::units::si::plane_angle> latitude,
               boost::units::quantity<boost::units::si::plane_angle> longitude,
               boost::units::quantity<boost::units::si::length> radius);

} // namespace orbit
} // namespace icarus
} // namespace ns3

#endif