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
#include "ns3/channel.h"
#include "ns3/log.h"
#include "ns3/abort.h"

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

GroundSatChannel::GroundSatChannel () : Channel ()
{
}

GroundSatChannel::~GroundSatChannel ()
{
}

std::size_t
GroundSatChannel::GetNDevices (void) const
{
  NS_ABORT_MSG ("Not implemented");
}

Ptr<NetDevice>
GroundSatChannel::GetDevice (std::size_t i) const
{
  NS_ABORT_MSG ("Not implemented");
}
} // namespace ns3