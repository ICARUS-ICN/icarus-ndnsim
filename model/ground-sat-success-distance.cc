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

#include "ground-sat-success-distance.h"
#include "ns3/abort.h"
#include "ns3/core-module.h"
#include "ns3/mobility-model.h"
#include "ns3/node.h"

namespace ns3 {
namespace icarus {

NS_LOG_COMPONENT_DEFINE ("icarus.GroundSatSuccessDistance");

NS_OBJECT_ENSURE_REGISTERED (GroundSatSuccessDistance);

const double GroundSatSuccessDistance::DEFAULT_MAX_DISTANCE = 1000000.0; // 1000km

TypeId
GroundSatSuccessDistance::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::icarus::GroundSatSuccessDistance")
          .SetParent<GroundSatSuccessModel> ()
          .SetGroupName ("ICARUS")
          .AddConstructor<GroundSatSuccessDistance> ()
          .AddAttribute ("MaxDistance",
                         "The maximum admissible transmission distance for successful transmission",
                         DoubleValue (DEFAULT_MAX_DISTANCE),
                         MakeDoubleAccessor (&GroundSatSuccessDistance::m_maxDistance),
                         MakeDoubleChecker<double> ());

  return tid;
}

GroundSatSuccessDistance::~GroundSatSuccessDistance ()
{
  NS_LOG_FUNCTION (this);
}

bool
GroundSatSuccessDistance::TramsmitSuccess (Ptr<Node> src, Ptr<Node> dst, Ptr<Packet>) const
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

} // namespace icarus
} // namespace ns3