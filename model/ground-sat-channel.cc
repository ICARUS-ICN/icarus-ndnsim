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
#include "ns3/mobility-model.h"
#include "ns3/net-device.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"
#include "sat2ground-net-device.h"
#include "ground-sta-net-device.h"
#include "ns3/assert.h"

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
  NS_ABORT_MSG_UNLESS (device->GetNode ()->GetObject<MobilityModel> () != 0,
                       "Satellites need a mobility model");
  m_satellites.Add (device);

  return true;
}

bool
GroundSatChannel::AttachGround (Ptr<GroundStaNetDevice> device)
{
  NS_ABORT_MSG_UNLESS (device->GetNode ()->GetObject<MobilityModel> () != 0,
                       "Ground stations need a mobility model");
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

Time
GroundSatChannel::Transmit2Sat (Ptr<Packet> packet, DataRate bps, uint16_t protocolNumber) const
{
  NS_ASSERT_MSG (m_ground, "We need a ground station");
  NS_ASSERT_MSG (m_satellites.GetN () == 1,
                 "WIP: Only 1 satellite en the constellation is supported");

  Time endTx = bps.CalculateBytesTxTime (packet->GetSize ());
  auto posGround = m_ground->GetNode ()->GetObject<MobilityModel> ();
  auto posSat = m_satellites.Get (0)->GetNode ()->GetObject<MobilityModel> ();
  auto distanceMeters = posGround->GetDistanceFrom (posSat);

  Time delay (Seconds (distanceMeters / 3e8));

  if (distanceMeters < 2000e3)
    {
      NS_LOG_WARN ("We should perform a real visibility test with the cone angle of the receiving "
                   "satellite");
      NS_ASSERT (DynamicCast<Sat2GroundNetDevice> (m_satellites.Get (0)) != 0);
      auto sat = StaticCast<Sat2GroundNetDevice> (m_satellites.Get (0));

      Simulator::ScheduleWithContext (sat->GetNode ()->GetId (), delay,
                                      &Sat2GroundNetDevice::ReceiveFromGround, sat, packet, bps,
                                      protocolNumber);
    }
  else
    {
      NS_LOG_DEBUG ("Dropped packet " << packet << " as distance " << distanceMeters / 1000.0
                                      << "km was too high.");
    }

  return endTx;
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