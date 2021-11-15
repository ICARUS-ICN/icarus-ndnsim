/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *
 * Copyright (c) 2021 Universidade de Vigo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Pablo Iglesias Sanuy <pabliglesias@alumnos.uvigo.es>
 *
 */

#include "sat2sat-success-model.h"
#include "satpos/planet.h"
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/core-module.h"
#include "ns3/mobility-model.h"
#include "ns3/node.h"

#include <boost/units/cmath.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/units/systems/si/length.hpp>

namespace ns3 {
namespace icarus {

NS_LOG_COMPONENT_DEFINE ("icarus.Sat2SatSuccessModel");

using namespace ::icarus::satpos::planet;
using constants::Earth;

NS_OBJECT_ENSURE_REGISTERED (Sat2SatSuccessModel);

const double Sat2SatSuccessModel::DEFAULT_MAX_DISTANCE = 2981438.0; // 2981.438km
const double Sat2SatSuccessModel::MIN_ALTITUDE_FOR_VISIBILITY = 80000.0; //80 km

TypeId
Sat2SatSuccessModel::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::icarus::Sat2SatSuccessModel").SetParent<Object> ()
      .SetGroupName ("ICARUS")
      .AddConstructor<Sat2SatSuccessModel> ();

  return tid;
}

Sat2SatSuccessModel::Sat2SatSuccessModel ()
{
  NS_LOG_FUNCTION(this);
}
Sat2SatSuccessModel::~Sat2SatSuccessModel ()
{
  NS_LOG_FUNCTION (this);
}

bool
Sat2SatSuccessModel::TramsmitSuccess (Ptr<Node> src, Ptr<Node> dst, Ptr<Packet>) const
{
  NS_LOG_FUNCTION (this << src << dst);

  const auto mobilitySrc = src->GetObject<MobilityModel> ();
  const auto mobilityDst = dst->GetObject<MobilityModel> ();

  NS_ABORT_MSG_UNLESS (mobilitySrc != nullptr, "Source node lacks location information.");
  NS_ABORT_MSG_UNLESS (mobilityDst != nullptr, "Destination node lacks location information.");

  const auto posSrc = mobilitySrc->GetPosition ();
  const auto posDst = mobilityDst->GetPosition ();

  return CalculateDistance (posSrc, posDst) <= m_maxDistance;
}

void
Sat2SatSuccessModel::CalcMaxDistance(double altitude)
{
  auto h = altitude; //Radius of the satellite
  auto r = MIN_ALTITUDE_FOR_VISIBILITY + Earth.getRadius().value(); // 80 km + Earth Radius
  m_maxDistance = 2 * sqrt((h*h) - (r*r)); //max distance = 2*sqrt(h^2 - r^2)
}

} // namespace icarus
} // namespace ns3