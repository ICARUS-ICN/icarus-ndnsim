/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Sergio Herrería Alonso <sha@det.uvigo.es>
 */

#include "poisson-helper.h"

#include "ns3/double.h"
#include "ns3/object.h"
#include "ns3/on-off-helper.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include <memory>

namespace ns3 {
namespace icarus {

const DataRate PoissonHelper::POISSON_MAX_DATA_RATE{"1Gbps"};

PoissonHelper::PoissonHelper (const std::string &protocol, const Address &address,
                              DataRate poissonRate, uint32_t packetSize,
                              uint32_t headerSize) noexcept

    : m_impl (std::make_unique<OnOffHelper> (protocol, address))
{
  auto ctrVariable = CreateObject<ConstantRandomVariable> ();
  const double t_on = packetSize * 8.0 / POISSON_MAX_DATA_RATE.GetBitRate ();
  ctrVariable->SetAttribute ("Constant", DoubleValue (t_on));

  auto expVariable = CreateObject<ExponentialRandomVariable> ();
  const double t_off = 8.0 * (packetSize + headerSize) / poissonRate.GetBitRate () - t_on;
  expVariable->SetAttribute ("Mean", DoubleValue (t_off));
  expVariable->SetAttribute ("Bound", DoubleValue (0.0));

  m_impl->SetAttribute ("PacketSize", UintegerValue (packetSize));
  m_impl->SetAttribute ("OnTime", PointerValue (ctrVariable));
  m_impl->SetAttribute ("OffTime", PointerValue (expVariable));
}

void
PoissonHelper::SetAttribute (const std::string &name, const AttributeValue &value) noexcept
{
  m_impl->SetAttribute (name, value);
}

ApplicationContainer
PoissonHelper::Install (Ptr<Node> node) const
{
  return m_impl->Install (node);
}

ApplicationContainer
PoissonHelper::Install (const std::string &nodeName) const
{
  return m_impl->Install (nodeName);
}

ApplicationContainer
PoissonHelper::Install (const NodeContainer &c) const
{
  return m_impl->Install (c);
}

int64_t
PoissonHelper::AssignStreams (const NodeContainer &c, int64_t stream)
{
  return m_impl->AssignStreams (c, stream);
}

} // namespace icarus
} // namespace ns3
