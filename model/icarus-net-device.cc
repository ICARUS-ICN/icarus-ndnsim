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

#include "icarus-net-device.h"
#include "ns3/log-macros-enabled.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("IcarusNetDevice");

NS_OBJECT_ENSURE_REGISTERED (IcarusNetDevice);

TypeId
IcarusNetDevice::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::IcarusNetDevice").SetParent<NetDevice> ().SetGroupName ("ICARUS");

  return tid;
}

IcarusNetDevice::~IcarusNetDevice ()
{
  NS_LOG_FUNCTION (this);
}
} // namespace ns3
