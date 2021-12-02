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
#include "ns3/string.h"
#include "ns3/data-rate.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/random-variable-stream.h"
#include "ns3/onoff-application.h"
#include "ns3/double.h"

namespace ns3 {

PoissonHelper::PoissonHelper (std::string protocol, Address address)
{
  m_factory.SetTypeId ("ns3::OnOffApplication");
  m_factory.Set ("Protocol", StringValue (protocol));
  m_factory.Set ("Remote", AddressValue (address));
  m_factory.Set ("DataRate", DataRateValue (POISSON_MAX_DATA_RATE));
}

void
PoissonHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
PoissonHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
PoissonHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
PoissonHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
PoissonHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<Application> ();
  node->AddApplication (app);

  return app;
}

int64_t
PoissonHelper::AssignStreams (NodeContainer c, int64_t stream)
{
  int64_t currentStream = stream;
  Ptr<Node> node;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      node = (*i);
      for (uint32_t j = 0; j < node->GetNApplications (); j++)
        {
          Ptr<OnOffApplication> onoff = DynamicCast<OnOffApplication> (node->GetApplication (j));
          if (onoff)
            {
              currentStream += onoff->AssignStreams (currentStream);
            }
        }
    }
  return (currentStream - stream);
}

void
PoissonHelper::SetPoissonRate (DataRate poissonRate, uint32_t packetSize)
{
  m_factory.Set ("PacketSize", UintegerValue (packetSize));
  DoubleValue t_on = DoubleValue (packetSize * 8.0 / POISSON_MAX_DATA_RATE);
  uint32_t header_size = 28; // IP + UDP headers size
  DoubleValue t_off =
      DoubleValue ((packetSize + header_size) * 8.0 / poissonRate.GetBitRate () - t_on.Get ());
  m_factory.Set ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=" +
                                        t_on.SerializeToString (NULL) + "]"));
  m_factory.Set ("OffTime", StringValue ("ns3::ExponentialRandomVariable[Mean=" +
                                         t_off.SerializeToString (NULL) + "|Bound=0]"));
}

} // namespace ns3
