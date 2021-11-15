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

#include "sat-address.h"

#include "ns3/address.h"
#include "ns3/assert.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include <cstring>
#include <iomanip>
#include <ios>
#include <netinet/in.h>
#include <sstream>

namespace ns3 {
namespace icarus {

NS_LOG_COMPONENT_DEFINE ("icarus.SatAddress");

ATTRIBUTE_HELPER_CPP (SatAddress);

const uint8_t SatAddress::m_type = Address::Register ();

uint8_t
SatAddress::GetType ()
{
  NS_LOG_FUNCTION_NOARGS ();

  return m_type;
}

SatAddress::SatAddress () : m_constellationId (0), m_orbitalPlane (0), m_planeIndex (0)
{
}

SatAddress::SatAddress (uint16_t constellationId, uint16_t orbitalPlane, uint16_t planeIndex)
    : m_constellationId (htons (constellationId)),
      m_orbitalPlane (htons (orbitalPlane)),
      m_planeIndex (htons (planeIndex))
{
}

Address
SatAddress::ConvertTo () const
{
  NS_LOG_FUNCTION (this);

  uint8_t buffer[WIRE_SIZE];

  std::memcpy (buffer, &m_constellationId, 2);
  std::memcpy (buffer + 2, &m_orbitalPlane, 2);
  std::memcpy (buffer + 4, &m_planeIndex, 2);

  return Address (GetType (), buffer, WIRE_SIZE);
}

void
SatAddress::CopyFrom (const uint8_t buffer[WIRE_SIZE])
{
  NS_LOG_FUNCTION (this << buffer);

  std::memcpy (&m_constellationId, buffer, 2);
  std::memcpy (&m_orbitalPlane, buffer + 2, 2);
  std::memcpy (&m_planeIndex, buffer + 4, 2);
}

void
SatAddress::CopyTo (uint8_t buffer[6]) const
{
  NS_LOG_FUNCTION (this << buffer);

  std::memcpy (buffer, &m_constellationId, 2);
  std::memcpy (buffer + 2, &m_orbitalPlane, 2);
  std::memcpy (buffer + 4, &m_planeIndex, 2);
}

SatAddress
SatAddress::ConvertFrom (const Address &address)
{
  NS_LOG_FUNCTION (address);

  NS_ASSERT (address.CheckCompatible (GetType (), WIRE_SIZE));

  uint8_t buffer[WIRE_SIZE];

  NS_ASSERT (address.GetLength () == WIRE_SIZE); // Just to be extra sure!
  address.CopyTo (buffer);

  const uint16_t constellation = (buffer[0] << 8) + buffer[1];
  const uint16_t plane = (buffer[2] << 8) + buffer[3];
  const uint16_t index = (buffer[4] << 8) + buffer[5];

  return SatAddress (constellation, plane, index);
}

uint16_t
SatAddress::getConstellationId () const
{
  NS_LOG_FUNCTION (this);

  return ntohs (m_constellationId);
}

uint16_t
SatAddress::getOrbitalPlane () const
{
  NS_LOG_FUNCTION (this);

  return ntohs (m_orbitalPlane);
}

uint16_t
SatAddress::getPlaneIndex () const
{
  NS_LOG_FUNCTION (this);

  return ntohs (m_planeIndex);
}

bool
operator== (const SatAddress &a, const SatAddress &b)
{
  NS_LOG_FUNCTION (a << b);

  return a.getConstellationId () == b.getConstellationId () &&
         a.getOrbitalPlane () == b.getOrbitalPlane () && a.getPlaneIndex () == b.getPlaneIndex ();
}

bool
operator!= (const SatAddress &a, const SatAddress &b)
{
  return !(a == b);
}

std::ostream &
operator<< (std::ostream &os, const SatAddress &address)
{
  NS_LOG_FUNCTION (&os << &address);

  os.setf (std::ios::hex, std::ios::basefield);
  os.fill ('0');

  os << std::setw (4) << address.getConstellationId () << ':';
  os << std::setw (4) << address.getOrbitalPlane () << ':';
  os << std::setw (4) << address.getPlaneIndex ();

  os.setf (std::ios::dec, std::ios::basefield);
  os.fill (' ');

  return os;
}

std::istream &
operator>> (std::istream &is, SatAddress &address)
{
  NS_LOG_FUNCTION (&is);

  uint16_t constellation, plane, index;

  std::string tmp;

  std::getline (is, tmp, ':');
  std::istringstream (tmp) >> std::hex >> constellation;
  std::getline (is, tmp, ':');
  std::istringstream (tmp) >> std::hex >> plane;
  is >> std::hex >> index;

  address = SatAddress (constellation, plane, index);

  return is;
}

} // namespace icarus
} // namespace ns3