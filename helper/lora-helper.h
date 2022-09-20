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

#ifndef LORA_HELPER_H
#define LORA_HELPER_H

#include "ns3/application-container.h"
#include "ns3/on-off-helper.h"
#include <memory>

namespace ns3 {

class Address;
class AttributeValue;
class DataRate;
class NodeContainer;

namespace icarus {

/**
 * \ingroup onoff
 * \brief A helper to make it easier to instantiate an ns3::OnOffApplication 
 * simulating LoRa traffic on a set of nodes.
 */
class LoraHelper
{
public:
  /**
   * Create a LoraHelper to make it easier to work with LoRa traffic.
   *
   * \param protocol The name of the protocol to use to send traffic
   *        by the applications. This string identifies the socket
   *        factory type used to create sockets for the applications.
   *        A typical value would be ns3::Ipv4RawSocketFactory.
   * \param address The address of the remote node to send traffic to.
   * \param spreadingFactor The LoRa spreading factor (7..12).
   * \param codingRate The LoRa coding rate (5..8).
   * \param bandwith The LoRa bandwidth (in kHz).
   * \param sendingRate DataRate object for the LoRa sending rate.
   * \param preambleSize Size in symbols of the LoRa preamble. 
   * \param headerSize Size in bytes of the protocol header.
   * \param payloadSize Size in bytes of the payload.
   * \param maxFrames Total number of LoRa frames to send (0 = no limit).
   */
  LoraHelper (const std::string &protocol, const Address &address, uint8_t spreadingFactor = 10u,
              uint8_t codingRate = 5u, uint8_t bandwidth = 125,
              DataRate sendingRate = DataRate (500), uint16_t preambleSize = 8u,
              uint16_t headerSize = 20u, uint32_t payloadSize = 51u,
              uint64_t maxFrames = 0u) noexcept;

  /**
   * Helper function used to set the underlying application attributes.
   *
   * \param name The name of the application attribute to set
   * \param value The value of the application attribute to set
   */
  void SetAttribute (const std::string &name, const AttributeValue &value) noexcept;

  /**
   * Install an ns3::OnOffApplication simulating LoRa traffic on each 
   * node of the input container configured with all the attributes set 
   * with SetAttribute.
   *
   * \param c NodeContainer of the set of nodes on which an OnOffApplication 
   * will be installed.
   * \returns Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (const NodeContainer &c) const;

  /**
   * Install an ns3::OnOffApplication simulating Poisson traffic on the node 
   * configured with all the attributes set with SetAttribute.
   *
   * \param node The node on which an OnOffApplication will be installed.
   * \returns Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (Ptr<Node> node) const;

  /**
   * Install an ns3::OnOffApplication simulating LoRa traffic on the node 
   * configured with all the attributes set with SetAttribute.
   *
   * \param nodeName The node on which an OnOffApplication will be installed.
   * \returns Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (const std::string &nodeName) const;

  /**
  * Assign a fixed random variable stream number to the random variables
  * used by this model. Return the number of streams (possibly zero) that
  * have been assigned. The Install() method should have previously been
  * called by the user.
  *
  * \param stream First stream index to use
  * \param c NodeContainer of the set of nodes for which the OnOffApplication
  *          should be modified to use a fixed stream
  * \return the number of stream indices assigned by this helper
  */
  int64_t AssignStreams (const NodeContainer &c, int64_t stream);

  /**
   * Returns the actual payload size of Lora frames.
   */
  uint32_t GetLoraPayloadSize () const;

  /**
   * Return the data rate of a Lora link with given parameters.
   *
   * \param spreadingFactor The LoRa spreading factor (7..12).
   * \param codingRate The LoRa coding rate (5..8).
   * \param bandwith The LoRa bandwidth (in kHz).
   */
  static DataRate GetLoraRate (uint8_t spreadingFactor, uint8_t codingRate, uint8_t bandwidth);

private:
  std::unique_ptr<OnOffHelper> m_impl;
  uint32_t loraPayloadSize;
};

} // namespace icarus
} // namespace ns3

#endif /* LORA_HELPER_H */
