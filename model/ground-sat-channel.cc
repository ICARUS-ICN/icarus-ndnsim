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

#include "ground-sat-channel.h"
#include "ns3/assert.h"
#include "ns3/channel.h"
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/net-device.h"
#include "sat2ground-net-device.h"
#include "ground-sta-net-device.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GroundSatChannel");

NS_OBJECT_ENSURE_REGISTERED (GroundSatChannel);

TypeId
GroundSatChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GroundSatChannel")
                          .SetParent<Channel> ()
                          .SetGroupName ("ICARUS")
                          .AddConstructor<GroundSatChannel> ();

  return tid;
}

bool
GroundSatChannel::AttachNewSat (Ptr<Sat2GroundNetDevice> device)
{
  m_satellites.Add (device);

  return true;
}

bool
GroundSatChannel::AttachGround (Ptr<GroundStaNetDevice> device)
{
  if (m_ground == 0)
    {
      m_ground = device;
      return true;
    }

  return false;
}

GroundSatChannel::GroundSatChannel () : Channel ()
{
}

GroundSatChannel::~GroundSatChannel ()
{
}

std::size_t
GroundSatChannel::GetNDevices (void) const
{
  return m_satellites.GetN () + (m_ground ? 1 : 0);
}

Ptr<NetDevice>
GroundSatChannel::GetDevice (std::size_t i) const
{
  // Last device is ground station
  NS_ABORT_MSG_UNLESS (i < GetNDevices (),
                       "Asking for " << i << "-th device of a total of " << GetNDevices ());

  if (i < m_satellites.GetN ())
    {
      return m_satellites.Get (i);
    }
  NS_ASSERT (i + 1 == GetNDevices ());
  NS_ASSERT (m_ground);

  return m_ground;
}
} // namespace ns3