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
#ifndef CONSTELLATION_HELPER_H
#define CONSTELLATION_HELPER_H

#include "ns3/constellation.h"

#include "ns3/object-factory.h"
#include "ns3/ptr.h"

#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/plane_angle.hpp>

namespace ns3 {
namespace icarus {

class IcarusHelper;

class ConstellationHelper
{
public:
  ConstellationHelper (Ptr<IcarusHelper> creator);
  ConstellationHelper (const ConstellationHelper &) = delete;

  Ptr<Constellation>
  CreateConstellation (boost::units::quantity<boost::units::si::length> altitude,
                       boost::units::quantity<boost::units::si::plane_angle> inclination,
                       unsigned n_planes, unsigned n_satellites_per_plane, unsigned n_phases);

private:
  Ptr<IcarusHelper> m_icarusHelper;
  ObjectFactory m_circularOrbitFactory;
};

} // namespace icarus
} // namespace ns3

#endif
