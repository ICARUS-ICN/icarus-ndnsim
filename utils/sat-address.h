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

#ifndef SAT_ADDRESS_H
#define SAT_ADDRESS_H

#include "ns3/address.h"

namespace ns3 {
namespace icarus {

class SatAddress
{
public:
  SatAddress ();
  SatAddress (uint16_t constellationId, uint16_t orbitalPlane, uint16_t planeIndex);

  Address ConvertTo () const;
  static SatAddress ConvertFrom (const Address &address);

  uint16_t getConstellationId () const;
  uint16_t getOrbitalPlane () const;
  uint16_t getPlaneIndex () const;

private:
  const static uint8_t m_type;
  constexpr static std::size_t WIRE_SIZE = 6;

  uint16_t m_constellationId; // Stored in network byte order
  uint16_t m_orbitalPlane; // Stored in network byte order
  uint16_t m_planeIndex; // Stored in network byte order

  static uint8_t GetType ();
};

ATTRIBUTE_HELPER_HEADER (SatAddress);

bool operator== (const SatAddress &a, const SatAddress &b);
bool operator!= (const SatAddress &a, const SatAddress &b);
std::ostream &operator<< (std::ostream &os, const SatAddress &address);
std::istream &operator>> (std::istream &is, SatAddress &address);

} // namespace icarus
} // namespace ns3

#endif
