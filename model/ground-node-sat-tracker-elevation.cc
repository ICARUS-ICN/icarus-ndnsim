/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *
 * Copyright (c) 2022 Universidade de Vigo
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

#include "ground-node-sat-tracker-elevation.h"

#include "ground-sta-net-device.h"
#include "circular-orbit.h"
#include "ns3/nstime.h"
#include "sat2ground-net-device.h"
#include "ground-node-sat-tracker.h"
#include "constellation.h"
#include "ns3/mobility-model.h"
#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/node.h"
#include "ns3/assert.h"
#include "ns3/scheduler.h"
#include <algorithm>
#include <boost/units/quantity.hpp>
#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <tuple>
#include <utility>

namespace ns3 {
namespace icarus {

NS_LOG_COMPONENT_DEFINE ("icarus.GroundNodeSatTrackerElevation");

NS_OBJECT_ENSURE_REGISTERED (GroundNodeSatTrackerElevation);

using namespace boost::units;

TypeId
GroundNodeSatTrackerElevation::GetTypeId (void) noexcept
{
  static TypeId tid =
      TypeId ("ns3::icarus::GroundNodeSatTrackerElevation")
          .SetParent<GroundNodeSatTracker> ()
          .SetGroupName ("ICARUS")
          .AddConstructor<GroundNodeSatTrackerElevation> ()
          .AddAttribute ("MinElevation",
                         "The minimum elevation needed to track a satellite, in degrees",
                         DoubleValue (25.0),
                         MakeDoubleAccessor (&GroundNodeSatTrackerElevation::setElevation,
                                             &GroundNodeSatTrackerElevation::getElevation),
                         MakeDoubleChecker<double> ());

  return tid;
}

void
GroundNodeSatTrackerElevation::setElevation (double min_elevation) noexcept
{
  NS_LOG_FUNCTION (this << min_elevation);

  m_elevation = quantity<si::plane_angle> (min_elevation * degree::degrees);
}

double
GroundNodeSatTrackerElevation::getElevation () const noexcept
{
  NS_LOG_FUNCTION (this);

  return quantity<degree::plane_angle> (m_elevation).value ();
}

void
GroundNodeSatTrackerElevation::DoInitialize ()
{
  NS_LOG_FUNCTION (this);

  // Chain up initalization
  GroundNodeSatTracker::DoInitialize ();

  Simulator::ScheduleNow (&GroundNodeSatTrackerElevation::Update, this);
}

std::vector<std::tuple<Time, std::size_t, std::size_t>>
GroundNodeSatTrackerElevation::getVisibleSats () const noexcept
{
  NS_LOG_FUNCTION (this);

  auto mmodel = GetObject<Node> ()->GetObject<MobilityModel> ();
  NS_ABORT_MSG_UNLESS (mmodel != nullptr, "Source node lacks location information.");
  const Vector pos = mmodel->GetPosition ();

  std::vector<std::tuple<Time, std::size_t, std::size_t>> satellites;

  for (auto plane = 0u; plane < GetConstellation ()->GetNPlanes (); plane++)
    {
      for (auto index = 0u; index < GetConstellation ()->GetPlaneSize (); index++)
        {
          const auto satmmodel = GetConstellation ()
                                     ->GetSatellite (plane, index)
                                     ->GetNode ()
                                     ->GetObject<CircularOrbitMobilityModel> ();
          const auto sat_elevation = satmmodel->getSatElevation (pos);
          if (sat_elevation > m_elevation)
            {
              NS_LOG_DEBUG ("Considering ("
                            << plane << ", " << index << ") at elevation "
                            << quantity<degree::plane_angle> (sat_elevation).value () << "°");

              const Time orbitalPeriod = satmmodel->getOrbitalPeriod ();
              const auto bye_time =
                  satmmodel->tryGetNextTimeAtElevation (m_elevation, GetObject<Node> ());

              // Visible satellites may be existing the visibility cone. Check that the next time at the
              // cone border is *soon*. Lets say less than half an orbit away.
              // FIXME: This can be improved by not searching for those late crossings in the first place, but lets keep this simple
              // for the time being.
              if (bye_time.has_value () && *bye_time - Simulator::Now () < orbitalPeriod / 2.0)
                { // Discard satellites that are leaving

                  NS_LOG_DEBUG ("Sat: (" << plane << ", " << index << ") will be visible for "
                                         << (*bye_time - Simulator::Now ()).GetSeconds () << "s.");

                  satellites.push_back (
                      std::make_tuple (*bye_time - Simulator::Now (), plane, index));
                }
            }
        }
    }

  NS_LOG_DEBUG ("Returning " << satellites.size () << " visible satellites");

  return satellites;
}

void
GroundNodeSatTrackerElevation::Update () noexcept
{
  NS_LOG_FUNCTION (this);

  // 1.- Find the set of visible satellites
  const auto visible_sats = getVisibleSats ();

  satsAvailable (visible_sats);

  // 2.- Choose the one with the longest remaining visibility
  const auto best = std::max_element (
      visible_sats.cbegin (), visible_sats.cend (),
      [] (const auto &a, const auto &b) { return std::get<0> (a) < std::get<0> (b); });

  if (best != visible_sats.cend ())
    {
      Time visibility_time;
      std::size_t plane, index;

      std::tie (visibility_time, plane, index) = *best;
      const Address remoteAddress (GetConstellation ()->GetSatellite (plane, index)->GetAddress ());
      GetNetDevice ()->SetRemoteAddress (remoteAddress);
      NS_LOG_DEBUG ("Tracking satellite " << remoteAddress);

      Simulator::Schedule (visibility_time, &GroundNodeSatTrackerElevation::Update, this);
    }
  else
    {
      NS_ABORT_MSG ("Could not find any visible satellite.");
    }
}

} // namespace icarus
} // namespace ns3
