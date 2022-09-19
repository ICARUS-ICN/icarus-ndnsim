/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Sergio Herrería Alonso <sha@det.uvigo.es>
 */

#include "lora-helper.h"

namespace ns3 {
namespace icarus {

LoraHelper::LoraHelper (const std::string &protocol, const Address &address,
                        uint8_t spreadingFactor, uint8_t codingRate, uint8_t bandwidth,
                        DataRate sendingRate, uint16_t preambleSize, uint16_t headerSize,
                        uint32_t payloadSize) noexcept

    : m_impl (std::make_unique<OnOffHelper> (protocol, address))
{
  NS_ASSERT_MSG (spreadingFactor >= 7 && spreadingFactor <= 12, "Invalid LoRa spreading factor");
  NS_ASSERT_MSG (codingRate >= 5 && codingRate <= 8, "Invalid LoRa coding rate");
  NS_ASSERT_MSG (bandwidth >= 8 && bandwidth <= 500, "Invalid LoRa bandwith");
  NS_ASSERT_MSG (preambleSize >= 6 && preambleSize <= 65532, "Invalid LoRa preamble size");

  double t_sym = pow (2, spreadingFactor) / bandwidth / 1000;
  uint8_t de = spreadingFactor <= 10 ? 0 : 1;
  uint32_t phySize = 8 + codingRate * ceil ((44 + 8 * payloadSize - 4 * spreadingFactor) / 4.0 /
                                            (spreadingFactor - 2 * de));
  double toa = t_sym * (preambleSize + phySize + 4.25);
  uint32_t loraPayloadSize = round (spreadingFactor * toa / codingRate / t_sym / 2) - headerSize;
  DataRate loraSendingRate =
      DataRate (sendingRate.GetBitRate () * loraPayloadSize / (loraPayloadSize + headerSize));

  m_impl->SetConstantRate (loraSendingRate, loraPayloadSize);
}

void
LoraHelper::SetAttribute (const std::string &name, const AttributeValue &value) noexcept
{
  m_impl->SetAttribute (name, value);
}

ApplicationContainer
LoraHelper::Install (Ptr<Node> node) const
{
  return m_impl->Install (node);
}

ApplicationContainer
LoraHelper::Install (const std::string &nodeName) const
{
  return m_impl->Install (nodeName);
}

ApplicationContainer
LoraHelper::Install (const NodeContainer &c) const
{
  return m_impl->Install (c);
}

int64_t
LoraHelper::AssignStreams (const NodeContainer &c, int64_t stream)
{
  return m_impl->AssignStreams (c, stream);
}

DataRate
LoraHelper::GetLoraRate (uint8_t spreadingFactor, uint8_t codingRate, uint8_t bandwidth)
{
  return DataRate (spreadingFactor * bandwidth * 4000 / pow (2, spreadingFactor) / codingRate);
}

} // namespace icarus
} // namespace ns3
