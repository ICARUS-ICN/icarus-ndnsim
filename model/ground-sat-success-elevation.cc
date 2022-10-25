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

bool
GroundSatSuccessElevation::TramsmitSuccess (const Ptr<Node> &src, const Ptr<Node> &dst,
                                            const Ptr<Packet> &) const noexcept
{
  using namespace boost::math::double_constants;

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

  const double elevation =
      satMobilityModel->getSatElevation (groundMobilityModel->GetPosition ()).value () * radian;

  return elevation >= m_minimumElevation;
}
} // namespace icarus
} // namespace ns3