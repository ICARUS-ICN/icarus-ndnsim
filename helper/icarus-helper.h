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
 * Author: Miguel Rodríguez Pérez <miguel@det.uvigo.gal>
 */
#ifndef ICARUS_HELPER_H
#define ICARUS_HELPER_H

#include "ndn-cxx/lp/geo-tag.hpp"
#include "ns3/icarus-module.h"

#include "ns3/simple-ref-count.h"
#include "ns3/trace-helper.h"
#include "ns3/ground-sat-channel.h"
#include "ns3/icarus-net-device.h"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include <functional>
#include <memory>

namespace ns3 {
namespace icarus {

class ConstellationHelper;

class IcarusHelper : public PcapHelperForDevice, public AsciiTraceHelperForDevice
{
public:
  /**
   * Construct a IcarusHelper.
   */
  IcarusHelper ();
  virtual ~IcarusHelper ()
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
   * ns3::icarus::GroundStaNetDevice or
   * ns3::icarus::Sat2GroundNetDevice created through IcarusHelper::Install.
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
   * ns3::icarus::GroundSatChannel created through IcarusHelper::Install.
   */
  void
  SetSuccessModel (std::string type, const std::string &n1 = "",
                   const AttributeValue &v1 = EmptyAttributeValue (), const std::string &n2 = "",
                   const AttributeValue &v2 = EmptyAttributeValue (), const std::string &n3 = "",
                   const AttributeValue &v3 = EmptyAttributeValue (), const std::string &n4 = "",
                   const AttributeValue &v4 = EmptyAttributeValue ());

  /**
   * \param type the type of MAC model
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
   * ns3::icarus::GroundSatChannel created through IcarusHelper::Install.
   */
  void SetMacModel (std::string type, const std::string &n1 = "",
                    const AttributeValue &v1 = EmptyAttributeValue (), const std::string &n2 = "",
                    const AttributeValue &v2 = EmptyAttributeValue (), const std::string &n3 = "",
                    const AttributeValue &v3 = EmptyAttributeValue (), const std::string &n4 = "",
                    const AttributeValue &v4 = EmptyAttributeValue ());

  void SetTrackerModel (std::string type, const std::string &n1, const AttributeValue &v1,
                        const std::string &n2, const AttributeValue &v2, const std::string &n3,
                        const AttributeValue &v3, const std::string &n4, const AttributeValue &v4);

  /**
   * \param n1 the name of the attribute to set \param v1 the value of the
   * attribute to set
   *
   * Set these attributes on each ns3::icarus::GroundStaNetDevice or
   * ns3::icarus::Sat2GroundNetDevice created by IcarusHelper::Install
   */
  void SetDeviceAttribute (const std::string &n1, const AttributeValue &v1);

  /**
   * \param n1 the name of the attribute to set \param v1 the value of the
   * attribute to set
   *
   * Set these attributes on each ns3::GroundSatChannel created by
   * IcarusHelper::Install
   */
  void SetChannelAttribute (const std::string &n1, const AttributeValue &v1);

  void FixNdnStackHelper (ndn::StackHelper &sh);
  /**
   * This method creates an ns3::icarus::GroundStaNetDevice or
   * ns3::icarus::Sat2GroundNetDevice with the attributes configured by
   * IcarusHelper::SetDeviceAttribute and then adds the device to the node and
   * attaches the provided channel to the device.
   *
   * \param node The node to install the device in \param channel The channel to
   * attach to the device. \returns A container holding the added net device.
   */
  NetDeviceContainer Install (Ptr<Node> node, Ptr<GroundSatChannel> channel,
                              ConstellationHelper &chelper) const;

  /**
   * This method creates an ns3::icarus::GroundStaNetDevice or
   * ns3::icarus::Sat2GroundNetDevice with the attributes configured by
   * IcarusHelper::SetDeviceAttribute and then adds the device to the node and
   * attaches the provided channel to the device.
   *
   * \param node The node to install the device in \param channelName The name
   * of the channel to attach to the device. \returns A container holding the
   * added net device.
   */
  NetDeviceContainer Install (Ptr<Node> node, const std::string &channelName,
                              ConstellationHelper &chelper) const;

  /**
   * This method creates an ns3::icarus::GroundStaNetDevice or
   * ns3::icarus::Sat2GroundNetDevice with the attributes configured by
   * IcarusHelper::SetDeviceAttribute and then adds the device to the node and
   * attaches the provided channel to the device.
   *
   * \param nodeName The name of the node to install the device in \param
   * channel The channel to attach to the device. \returns A container holding
   * the added net device.
   */
  NetDeviceContainer Install (const std::string &nodeName, Ptr<GroundSatChannel> channel,
                              ConstellationHelper &chelper) const;

  /**
   * This method creates an ns3::icarus::GroundStaNetDevice or
   * ns3::icarus::Sat2GroundNetDevice with the attributes configured by
   * IcarusHelper::SetDeviceAttribute and then adds the device to the node and
   * attaches the provided channel to the device.
   *
   * \param nodeName The name of the node to install the device in \param
   * channelName The name of the channel to attach to the device. \returns A
   * container holding the added net device.
   */
  NetDeviceContainer Install (const std::string &nodeName, const std::string &channelName,
                              ConstellationHelper &chelper) const;

  /**
   * This method creates an ns3::GroundSatChannel with the attributes configured
   * by IcarusHelper::SetChannelAttribute. For each Ptr<node> in the provided
   * container: it creates an ns3::icarus::GroundStaNetDevice or
   * ns3::icarus::Sat2GroundNetDevice (with the attributes configured by
   * IcarusHelper::SetDeviceAttribute); adds the device to the node; and
   * attaches the channel to the device.
   *
   * \param c The NodeContainer holding the nodes to be changed. \returns A
   * container holding the added net devices.
   */
  NetDeviceContainer Install (const NodeContainer &c, ConstellationHelper &chelper) const;

  /**
   * For each Ptr<node> in the provided container, this method creates an
   * ns3::icarus::GroundStaNetDevice or ns3::icarus::Sat2GroundNetDevice (with
   * the attributes configured by IcarusHelper::SetDeviceAttribute); adds the
   * device to the node; and attaches the provided channel to the device.
   *
   * \param c The NodeContainer holding the nodes to be changed. \param channel
   * The channel to attach to the devices. \returns A container holding the
   * added net devices.
   */
  NetDeviceContainer Install (const NodeContainer &c, Ptr<GroundSatChannel> channel,
                              ConstellationHelper &chelper) const;

  /**
   * For each Ptr<node> in the provided container, this method creates an
   * ns3::icarus::GroundStaNetDevice or ns3::icarus::Sat2GroundNetDevice (with
   * the attributes configured by IcarusHelper::SetDeviceAttribute); adds the
   * device to the node; and attaches the provided channel to the device.
   *
   * \param c The NodeContainer holding the nodes to be changed. \param
   * channelName The name of the channel to attach to the devices. \returns A
   * container holding the added net devices.
   */
  NetDeviceContainer Install (const NodeContainer &c, const std::string &channelName,
                              ConstellationHelper &chelper) const;

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

  void SetEnableGeoTags (std::function<std::shared_ptr<ndn::lp::GeoTag> ()> enableGeoTags);

private:
  /**
   * This method creates an ns3::icarus::GroundStaNetDevice or
   * ns3::icarus::Sat2GroundNetDevice with the attributes configured by
   * CsmaHelper::SetDeviceAttribute and then adds the device to the node and
   * attaches the provided channel to the device.
   *
   * \param node The node to install the device in \param channel The channel to
   * attach to the device. \returns A container holding the added net device.
   */
  Ptr<NetDevice> InstallPriv (Ptr<Node> node, Ptr<GroundSatChannel> channel,
                              ConstellationHelper &chelper) const;

  Ptr<IcarusNetDevice> CreateDeviceForNode (Ptr<Node> node, ConstellationHelper &chelper) const;

  std::string constructFaceUri (Ptr<NetDevice> netDevice);

  std::shared_ptr<nfd::face::Face>
  GroundStaNetDeviceCallback (Ptr<Node> node, Ptr<ndn::L3Protocol> ndn, Ptr<NetDevice> netDevice);

  std::shared_ptr<nfd::face::Face>
  Sat2GroundNetDeviceCallback (Ptr<Node> node, Ptr<ndn::L3Protocol> ndn, Ptr<NetDevice> netDevice);

  ObjectFactory m_queueFactory; //!< factory for the queues
  ObjectFactory m_sat2GroundFactory; //!< factory for downstream NetDevices
  ObjectFactory m_groundStaFactory; //!< factory for downstream NetDevices
  ObjectFactory m_channelFactory; //!< factory for the channel
  ObjectFactory m_successModelFactory; //!> factory for the success models
  ObjectFactory m_macModelFactory; //!> factory for the MAC models
  ObjectFactory m_trackerModelFactory; //!> factory for the Tracker models

  std::function<std::shared_ptr<ndn::lp::GeoTag> ()> m_enableGeoTags;
};

} // namespace icarus
} // namespace ns3

#endif /* ICARUS_HELPER_H */
