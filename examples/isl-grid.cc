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

#include "ns3/assert.h"
#include "ns3/command-line.h"
#include "ns3/icarus-module.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/node-container.h"
#include "ns3/simulator.h"
#include "ns3/geographic-positions.h"
#include "src/icarus/helper/isl-helper.h"

#include <boost/units/io.hpp>
#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/si/prefixes.hpp>
#include <boost/units/systems/si/length.hpp>
#include <cstddef>
#include <string>

NS_LOG_COMPONENT_DEFINE ("icarus.ISLGridExample");

namespace ns3 {
namespace icarus {

namespace {

void
logLinks (const Ptr<Constellation> &constellation)
{
  NS_LOG_FUNCTION (constellation);

  for (std::size_t plane = 0; plane < constellation->GetNPlanes (); plane++)
    {
      for (std::size_t index = 0; index < constellation->GetPlaneSize (); index++)
        {
          const auto &sat = constellation->GetSatellite (plane, index);
          const auto nlinks = sat->GetNode ()->GetNDevices ();
          NS_LOG_DEBUG ("Satellite: (" << plane << ", " << index << ") has "
                                       << std::to_string (nlinks) << " links");
        }
    }

  ns3::Simulator::Schedule (Seconds (1.0), logLinks, constellation);
}
} // namespace

auto
main (int argc, char **argv) -> int
{
  using namespace boost::units;
  using namespace boost::units::si;

  CommandLine cmd;

  cmd.Parse (argc, argv);

  IcarusHelper icarusHelper;
  ISLHelper islHelper;
  ConstellationHelper constellationHelper (quantity<length> (250 * kilo * meters),
                                           quantity<plane_angle> (60 * degree::degree), 6, 20, 1);

  NodeContainer nodes;
  nodes.Create (6 * 20);
  icarusHelper.Install (nodes, constellationHelper);
  islHelper.Install (nodes, constellationHelper);

  ns3::Simulator::Stop (Seconds (2));

  ns3::Simulator::Schedule (Seconds (0.0), logLinks, constellationHelper.GetConstellation ());

  ns3::Simulator::Run ();
  ns3::Simulator::Destroy ();

  return 0;
}
} // namespace icarus
} // namespace ns3

auto
main (int argc, char **argv) -> int
{
  return ns3::icarus::main (argc, argv);
}