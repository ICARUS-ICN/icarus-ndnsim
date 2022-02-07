/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Universidade de Vigo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Pablo Iglesias Sanuy <pabliglesias@alumnos.uvigo.es>
 */
#ifndef ISL_HELPER_H
#define ISL_HELPER_H

#include "ns3/icarus-module.h"

#include "ns3/simple-ref-count.h"
#include "ns3/trace-helper.h"
#include "ns3/sat2sat-channel.h"
#include "ns3/sat-net-device.h"
#include "src/ndnSIM/helper/ndn-stack-helper.hpp"

namespace ns3 {
namespace icarus {

class ISLHelper : public PcapHelperForDevice, public AsciiTraceHelperForDevice
{
public:
  /**
   * Construct a ISLHelper.
   */
  ISLHelper ();
  virtual ~ISLHelper ()
  {
  }

  /**
   * \param type the type of queue
   * \param n1 the name of the attribute to set on the queue
   * \param v1 the value of the attribute to set on the queue
   * \param n2 the name of the attribute to set on the queue
   * \param v2 the value of the attribute to set on the queue
   * \param n3 the name of the attribute to set on the queue
   * \param v3 the value of the attribute to set on the queue
   * \param n4 the name of the attribute to set on the queue
   * \param v4 the value of the attribute to set on the queue
   *
   * Set the type of queue to create and associated to each
   * ns3::icarus::SatNetDevice created through ISLHelper::Install.
   */
  void SetQueue (std::string type, const std::string &n1 = "",
                 const AttributeValue &v1 = EmptyAttributeValue (), const std::string &n2 = "",
                 const AttributeValue &v2 = EmptyAttributeValue (), const std::string &n3 = "",
                 const AttributeValue &v3 = EmptyAttributeValue (), const std::string &n4 = "",
                 const AttributeValue &v4 = EmptyAttributeValue ());

  /**
   * \param type the type of success model
   * \param n1 the name of the attribute to set on the queue
   * \param v1 the value of the attribute to set on the queue
   * \param n2 the name of the attribute to set on the queue
   * \param v2 the value of the attribute to set on the queue
   * \param n3 the name of the attribute to set on the queue
   * \param v3 the value of the attribute to set on the queue
   * \param n4 the name of the attribute to set on the queue
   * \param v4 the value of the attribute to set on the queue
   *
   * Set the type of success model to create and associated to each
   * ns3::icarus::Sat2SatChannel created through ISLHelper::Install.
   */
  void
  SetSuccessModel (std::string type, const std::string &n1 = "",
                   const AttributeValue &v1 = EmptyAttributeValue (), const std::string &n2 = "",
                   const AttributeValue &v2 = EmptyAttributeValue (), const std::string &n3 = "",
                   const AttributeValue &v3 = EmptyAttributeValue (), const std::string &n4 = "",
                   const AttributeValue &v4 = EmptyAttributeValue ());

  /**
   * \param n1 the name of the attribute to set \param v1 the value of the
   * attribute to set
   *
   * Set these attributes on each ns3::icarus::SatNetDevice created by
   * ISLHelper::Install
   */
  void SetDeviceAttribute (const std::string &n1, const AttributeValue &v1);

  /**
   * \param n1 the name of the attribute to set \param v1 the value of the
   * attribute to set
   *
   * Set these attributes on each ns3::Sat2SatChannel created by
   * ISLHelper::Install
   */
  void SetChannelAttribute (const std::string &n1, const AttributeValue &v1);

  void FixNdnStackHelper (ndn::StackHelper &sh);

  /**
   * This method creates four ns3::Sat2SatChannel for each node with the attributes
   * configured by ISLHelper::SetChannelAttribute. For each Ptr<Node> in the input
   * container: it creates an ns3::icarus::SatNetDevice (with the attributes
   * configured by ISLHelper::SetDeviceAttribute); adds the device to the node; and
   * attaches the four channels.
   * 
   * @param c The NodeContainer holding the nodes to be changed
   * @param chelper The constellation helper
   * @return NetDeviceContainer 
   */
  NetDeviceContainer Install (const NodeContainer &c, ConstellationHelper &chelper);

  /**
   * This method creates an ns3::Sat2SatChannel with the attributes configured by
   * ISLHelper::SetChannelAttribute. For each Ptr<node> in the input container:
   * it creates an ns3::icarus::SatNetDevice (with the attributes
   * configured by ISLHelper::SetDeviceAttribute); adds the device to the
   * node; and attaches the channel to both devices.
   *
   * \param c The NodeContainer holding the nodes to be changed. \returns A
   * container holding the added net devices.
   */
  NetDeviceContainer Install (const NodeContainer &c) const;

  /**
   * This method creates a point-to-point ns3::Sat2SatChannel for two nodes.
   * 
   * \param a first node;
   * \param b second node;
   * \returns A container holding the added netDevices. 
   */
  NetDeviceContainer Install (Ptr<Node> a, Ptr<Node> b) const;

  /**
   * \brief Enable pcap output on the indicated net device.
   *
   * NetDevice-specific implementation mechanism for hooking the trace and
   * writing to the trace file.
   *
   * \param prefix Filename prefix to use for pcap files. \param nd Net device
   * for which you want to enable tracing. \param promiscuous If true capture
   * all possible packets available at the device. \param explicitFilename Treat
   * the prefix as an explicit filename if true
   */
  virtual void EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous,
                                   bool explicitFilename);

  /**
   * \brief Enable ascii trace output on the indicated net device.
   *
   * NetDevice-specific implementation mechanism for hooking the trace and
   * writing to the trace file.
   *
   * \param stream The output stream object to use when logging ascii traces.
   * \param prefix Filename prefix to use for ascii trace files. \param nd Net
   * device for which you want to enable tracing. \param explicitFilename Treat
   * the prefix as an explicit filename if true
   */
  virtual void EnableAsciiInternal (Ptr<OutputStreamWrapper> stream, std::string prefix,
                                    Ptr<NetDevice> nd, bool explicitFilename);

private:
  /**
   * This method creates an ns3::icarus::SatNetDevice with the attributes
   * configured by ISLHelper::SetDeviceAttribute and then adds the device to
   * the node and attaches the provided channel to the device.
   *
   * \param node The node to install the device in \param channel The channel to
   * attach to the device. \returns A container holding the added net device.
   */
  Ptr<NetDevice> InstallPriv (Ptr<Node> node, Ptr<Sat2SatChannel> channel) const;

  std::string constructFaceUri (Ptr<NetDevice> netDevice);

  std::shared_ptr<nfd::face::Face> SatNetDeviceCallback (Ptr<Node> node, Ptr<ndn::L3Protocol> ndn,
                                                         Ptr<NetDevice> device);

  ObjectFactory m_queueFactory; //!< factory for the queues
  ObjectFactory m_satNetDeviceFactory; //!< factory for NetDevices
  ObjectFactory m_channelFactory; //!< factory for the channel
  ObjectFactory m_successModelFactory; //!> factory for the success models
};

} // namespace icarus
} // namespace ns3

#endif /* ISL_HELPER_H */
