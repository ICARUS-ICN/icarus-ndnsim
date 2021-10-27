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

#include "ground-sat-success-model.h"
#include "ns3/log.h"

namespace ns3 {
namespace icarus {

NS_LOG_COMPONENT_DEFINE ("icarus.GroundSatSuccessModel");

NS_OBJECT_ENSURE_REGISTERED (GroundSatSuccessModel);

TypeId
GroundSatSuccessModel::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::icarus::GroundSatSuccessModel").SetParent<Object> ().SetGroupName ("ICARUS");

  return tid;
}

GroundSatSuccessModel::~GroundSatSuccessModel ()
{
  NS_LOG_FUNCTION (this);
}

} // namespace icarus
} // namespace ns3