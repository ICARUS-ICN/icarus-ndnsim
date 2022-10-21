/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *
 * Copyright (c) 2021–2022 Universidade de Vigo
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

#include "ground-node-sat-tracker-periodic.h"

#include "ns3/constellation.h"
#include "ns3/ground-sta-net-device.h"
#include "ns3/mobility-model.h"
#include "ns3/random-variable-stream.h"
#include "ns3/sat2ground-net-device.h"
#include "ns3/simulator.h"

namespace ns3 {
namespace icarus {

NS_LOG_COMPONENT_DEFINE ("icarus.GroundNodeSatTrackerPeriodic");

NS_OBJECT_ENSURE_REGISTERED (GroundNodeSatTrackerPeriodic);

TypeId
GroundNodeSatTrackerPeriodic::GetTypeId (void) noexcept
{
  static TypeId tid =
      TypeId ("ns3::icarus::GroundNodeSatTrackerPeriodic")
          .SetParent<GroundNodeSatTracker> ()
          .SetGroupName ("ICARUS")
          .AddConstructor<GroundNodeSatTrackerPeriodic> ()
          .AddAttribute ("TrackingInterval",
                         "The amount of time between two consecutive antenna adjustments (0 "
                         "disables tracking)",
                         TimeValue (Seconds (0)),
                         MakeTimeAccessor (&GroundNodeSatTrackerPeriodic::m_interval),
                         MakeTimeChecker ());

  return tid;
}

void
GroundNodeSatTrackerPeriodic::Start () const noexcept
{
  NS_LOG_FUNCTION (this);

  if (m_interval == Seconds (0))
    {
      NS_LOG_DEBUG ("Tracker is disabled. Will not start.");

      return;
    }

  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  ns3::Simulator::ScheduleNow (&GroundNodeSatTrackerPeriodic::UpdateOnce, this);

  // Use a random point in the interval for the first update
  ns3::Simulator::Schedule (Seconds (x->GetValue (0.0, m_interval.GetSeconds ())),
                            &GroundNodeSatTrackerPeriodic::PeriodicUpdate, this);
}

void
GroundNodeSatTrackerPeriodic::PeriodicUpdate () const noexcept
{
  NS_LOG_FUNCTION (this);

  UpdateOnce ();

  ns3::Simulator::Schedule (m_interval, &GroundNodeSatTrackerPeriodic::PeriodicUpdate, this);
}

const Constellation *
GroundNodeSatTrackerPeriodic::GetConstellation () const noexcept
{
  NS_LOG_FUNCTION (this);

  if (m_constellation == nullptr)
    {
      m_constellation =
          DynamicCast<GroundSatChannel> (GetNetDevice ()->GetChannel ())->GetConstellation ();
    }

  return PeekPointer (m_constellation);
}

GroundStaNetDevice *
GroundNodeSatTrackerPeriodic::GetNetDevice () const noexcept
{
  NS_LOG_FUNCTION (this);

  if (m_netDevice == nullptr)
    {
      const auto node = GetObject<Node> ();

      for (auto i = 0u; i < node->GetNDevices (); i++)
        {
          const auto dev = DynamicCast<GroundStaNetDevice> (node->GetDevice (i));
          if (dev != nullptr)
            {
              m_netDevice = dev;
            }
        }
    }

  NS_ASSERT_MSG (m_netDevice != nullptr, "Node needs to have a GroundStaNetDevice.");

  return PeekPointer (m_netDevice);
}

Vector3D
GroundNodeSatTrackerPeriodic::GetPosition () const noexcept
{
  NS_LOG_FUNCTION (this);

  if (m_mobilityModel == nullptr)
    {
      m_mobilityModel = GetObject<Node> ()->GetObject<MobilityModel> ();
    }

  return m_mobilityModel->GetPosition ();
}

void
GroundNodeSatTrackerPeriodic::UpdateOnce () const noexcept
{
  NS_LOG_FUNCTION (this);

  const auto pos = GetPosition ();

  auto constellation = GetConstellation ();
  const auto remoteAddress = constellation->GetClosest (pos)->GetAddress ();
  GetNetDevice ()->SetRemoteAddress (remoteAddress);

  NS_LOG_DEBUG ("Tracking satellite " << remoteAddress);
}

} // namespace icarus
} // namespace ns3
