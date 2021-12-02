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

#ifndef POISSON_HELPER_H
#define POISSON_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/address.h"
#include "ns3/attribute.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"

namespace ns3 {

class DataRate;

#define POISSON_MAX_DATA_RATE 1e9

/**
 * \ingroup onoff
 * \brief A helper to make it easier to instantiate an ns3::OnOffApplication 
 * simulating Poisson traffic on a set of nodes.
 */
class PoissonHelper
{
public:
  /**
   * Create a PoissonHelper to make it easier to work with Poisson 
   * applications.
   *
   * \param protocol The name of the protocol to use to send traffic
   *        by the applications. This string identifies the socket
   *        factory type used to create sockets for the applications.
   *        A typical value would be ns3::UdpSocketFactory.
   * \param address The address of the remote node to send traffic to.
   */
  PoissonHelper (std::string protocol, Address address);

  /**
   * Helper function used to set the underlying application attributes.
   *
   * \param name The name of the application attribute to set
   * \param value The value of the application attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Install an ns3::OnOffApplication simulating Poisson traffic on each 
   * node of the input container configured with all the attributes set 
   * with SetAttribute.
   *
   * \param c NodeContainer of the set of nodes on which an OnOffApplication 
   * will be installed.
   * \returns Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (NodeContainer c) const;

  /**
   * Install an ns3::OnOffApplication simulating Poisson traffic on the node 
   * configured with all the attributes set with SetAttribute.
   *
   * \param node The node on which an OnOffApplication will be installed.
   * \returns Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (Ptr<Node> node) const;

  /**
   * Install an ns3::OnOffApplication simulating Poisson traffic on the node 
   * configured with all the attributes set with SetAttribute.
   *
   * \param nodeName The node on which an OnOffApplication will be installed.
   * \returns Container of Ptr to the applications installed.
   */
  ApplicationContainer Install (std::string nodeName) const;

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
  int64_t AssignStreams (NodeContainer c, int64_t stream);

  /**
   * Helper function to set the sending rate of the Poisson application.
   *
   * \param poissonRate DataRate object for the Poisson sending rate
   * \param packetSize Size in bytes of the packet payloads generated
   */
  void SetPoissonRate (DataRate poissonRate, uint32_t packetSize = 512);

private:
  /**
   * Install an ns3::OnOffApplication simulating Poisson traffic on the node 
   * configured with all the attributes set with SetAttribute.
   *
   * \param node The node on which an OnOffApplication will be installed.
   * \returns Ptr to the application installed.
   */
  Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_factory; //!< Object factory.
};

} // namespace ns3

#endif /* POISSON_HELPER_H */
