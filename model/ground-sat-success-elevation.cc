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

#include "ns3/abort.h"
#include "ns3/double.h"
#include "ns3/log-macros-disabled.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/node.h"
#include "ns3/vector.h"

#include <boost/math/constants/constants.hpp>
namespace ns3 {
namespace icarus {

NS_LOG_COMPONENT_DEFINE ("icarus.GroundSatSuccessElevation");

NS_OBJECT_ENSURE_REGISTERED (GroundSatSuccessElevation);

namespace {
using namespace boost::math::double_constants;
constexpr double MINIMUM_ELEVATION = 25 * degree;
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
              "successful communication",
              DoubleValue (MINIMUM_ELEVATION),
              MakeDoubleAccessor (&GroundSatSuccessElevation::GetMinimumElevationDegrees,
                                  &GroundSatSuccessElevation::SetMinimumElevationDegrees),
              MakeDoubleChecker<double> ());

  return tid;
}

double
GroundSatSuccessElevation::GetMinimumElevationDegrees () const noexcept
{
  using namespace boost::math::double_constants;

  return m_minimumElevation * radian;
}

void
GroundSatSuccessElevation::SetMinimumElevationDegrees (double minElevation) noexcept
{
  using namespace boost::math::double_constants;

  NS_LOG_FUNCTION (this << minElevation);

  m_minimumElevation = minElevation * degree;
}

namespace {
double
GetSqSum (const Vector3D &vec) noexcept
{
  return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
}

double
GetDotProduct (const Vector3D &vecA, const Vector3D &vecB) noexcept
{
  return vecA.x * vecB.x + vecA.y * vecB.y + vecA.z * vecB.z;
}

} // namespace

bool
GroundSatSuccessElevation::TramsmitSuccess (const Ptr<Node> &src, const Ptr<Node> &dst,
                                            const Ptr<Packet> &) const noexcept
{
  using namespace boost::math::double_constants;

  NS_LOG_FUNCTION (this << src << dst);

  const auto mobilitySrc = src->GetObject<MobilityModel> ();
  const auto mobilityDst = dst->GetObject<MobilityModel> ();

  NS_ABORT_MSG_UNLESS (mobilitySrc != nullptr, "Source node lacks location information.");
  NS_ABORT_MSG_UNLESS (mobilityDst != nullptr, "Destination node lacks location information.");

  auto posSrc = mobilitySrc->GetPosition ();
  auto posDst = mobilityDst->GetPosition ();

  // Check which position belongs to the satellite (the longest vector)
  Vector3D &posSat = posDst, &posGround = posSrc;
  if (GetSqSum (posSrc) > GetSqSum (posDst))
    {
      posSat = posSrc;
      posGround = posDst;
    }
  // Get the vector pointing from ground to the satellite
  const auto vectorD = posSat - posGround;

  /* The calculation comes from Stanley Q. Kidder, Thomas H. Vonder Haar, 2 -
   * Orbits and Navigation, Editor(s): Stanley Q. Kidder, Thomas H. Vonder Haar,
   * Satellite Meteorology, Academic Press, 1995, Pages 15-46, ISBN
   * 9780124064300, https://doi.org/10.1016/B978-0-08-057200-0.50006-7. 
   */
  const auto cosZenith =
      GetDotProduct (posGround, vectorD) / (posGround.GetLength () * vectorD.GetLength ());

  const double elevation = quarter_pi - acos (cosZenith);
  NS_LOG_DEBUG ("Elevation: " << elevation * radian << " at distance: " << vectorD.GetLength ());

  return elevation >= m_minimumElevation;
}
} // namespace icarus
} // namespace ns3