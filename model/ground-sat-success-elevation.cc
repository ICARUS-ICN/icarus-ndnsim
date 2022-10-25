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
 * Author: Miguel Rodríguez Pérez <miguel@det.uvigo.gal>
 *
 */

#include "ground-sat-success-elevation.h"

#include "circular-orbit.h"
#include "ns3/abort.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/node.h"
#include "ns3/vector.h"
#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/si/plane_angle.hpp>

namespace ns3 {
namespace icarus {

NS_LOG_COMPONENT_DEFINE ("icarus.GroundSatSuccessElevation");

NS_OBJECT_ENSURE_REGISTERED (GroundSatSuccessElevation);

using namespace boost::units;

namespace {
constexpr auto MINIMUM_ELEVATION (25.0 * degree::degrees);
} // namespace

TypeId
GroundSatSuccessElevation::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::icarus::GroundSatSuccessElevation")
          .SetParent<GroundSatSuccessModel> ()
          .SetGroupName ("ICARUS")
          .AddConstructor<GroundSatSuccessElevation> ()
          .AddAttribute (
              "MinElevation",
              "The minimum elevation of the satellite over the ground station for "
              "successful communication (in degrees)",
              DoubleValue (quantity<degree::plane_angle> (MINIMUM_ELEVATION).value ()),
              MakeDoubleAccessor (&GroundSatSuccessElevation::GetMinimumElevationDegrees,
                                  &GroundSatSuccessElevation::SetMinimumElevationDegrees),
              MakeDoubleChecker<double> ());

  return tid;
}

double
GroundSatSuccessElevation::GetMinimumElevationDegrees () const noexcept
{
  return quantity<degree::plane_angle> (m_minimumElevation).value ();
}

void
GroundSatSuccessElevation::SetMinimumElevationDegrees (double minElevation) noexcept
{
  NS_LOG_FUNCTION (this << minElevation);

  m_minimumElevation = quantity<si::plane_angle> (minElevation * degree::degrees);
}

bool
GroundSatSuccessElevation::TramsmitSuccess (const Ptr<Node> &src, const Ptr<Node> &dst,
                                            const Ptr<Packet> &) const noexcept
{
  NS_LOG_FUNCTION (this << src << dst);

  Ptr<MobilityModel> groundMobilityModel;
  Ptr<CircularOrbitMobilityModel> satMobilityModel;

  if (src->GetObject<CircularOrbitMobilityModel> ())
    {
      satMobilityModel = src->GetObject<CircularOrbitMobilityModel> ();
      groundMobilityModel = dst->GetObject<MobilityModel> ();
    }
  else
    {
      satMobilityModel = dst->GetObject<CircularOrbitMobilityModel> ();
      groundMobilityModel = src->GetObject<MobilityModel> ();
    }

  NS_ABORT_MSG_UNLESS (satMobilityModel != nullptr, "Source node lacks location information.");
  NS_ABORT_MSG_UNLESS (groundMobilityModel != nullptr,
                       "Destination node lacks location information.");

  const auto elevation = satMobilityModel->getSatElevation (groundMobilityModel->GetPosition ());

  return elevation >= m_minimumElevation;
}
} // namespace icarus
} // namespace ns3