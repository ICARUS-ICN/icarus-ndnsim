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

#include "ground-node-sat-tracker.h"

#include "constellation.h"

#include <ns3/log.h>
#include <ns3/node.h>
#include <ns3/ground-sat-channel.h>
#include <ns3/ground-sta-net-device.h>
#include <ns3/sat2ground-net-device.h>

namespace ns3 {
namespace icarus {

NS_LOG_COMPONENT_DEFINE ("icarus.GroundNodeSatTracker");

NS_OBJECT_ENSURE_REGISTERED (GroundNodeSatTracker);

TypeId
GroundNodeSatTracker::GetTypeId (void) noexcept
{
  static TypeId tid =
      TypeId ("ns3::icarus::GroundNodeSatTracker").SetParent<Object> ().SetGroupName ("ICARUS");

  return tid;
}

GroundNodeSatTracker::GroundNodeSatTracker () noexcept
    : m_constellation (nullptr), m_netDevice (nullptr)
{
}

const Constellation *
GroundNodeSatTracker::GetConstellation () const noexcept
{
  NS_LOG_FUNCTION (this);

  if (m_constellation == nullptr)
    {
      m_constellation = PeekPointer (
          DynamicCast<GroundSatChannel> (GetNetDevice ()->GetChannel ())->GetConstellation ());
    }

  return m_constellation;
}

GroundStaNetDevice *
GroundNodeSatTracker::GetNetDevice () const noexcept
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
              m_netDevice = PeekPointer (dev);
            }
        }
    }

  NS_ASSERT_MSG (m_netDevice != nullptr, "Node needs to have a GroundStaNetDevice.");

  return m_netDevice;
}

} // namespace icarus
} // namespace ns3
