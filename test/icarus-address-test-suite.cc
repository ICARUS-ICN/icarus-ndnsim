/* Copyright (c) 2021 Universidade de Vigo
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

#include "ns3/address.h"
#include "ns3/sat-address.h"

#include "ns3/test.h"
#include "ns3/core-module.h"
#include "utils/sat-address.h"
#include <cstring>

using namespace ns3;
using namespace icarus;

class SatAddressTestCase : public TestCase
{
public:
  SatAddressTestCase ();
  virtual ~SatAddressTestCase ();

private:
  virtual void DoRun (void) override;

  ObjectFactory m_circularOrbit;
};

void
SatAddressTestCase::DoRun ()
{
  static const uint8_t expected_buffer[] = {0xf0, 0x0f, 0xe1, 0x1e, 0xd2, 0x2d};

  uint8_t buffer[6];

  NS_TEST_ASSERT_MSG_EQ (sizeof (expected_buffer), 6, "Incorrect buffer length");
  const auto test = SatAddress (0xf00f, 0xe11e, 0xd22d);

  test.ConvertTo ().CopyTo (buffer);
  NS_TEST_ASSERT_MSG_EQ (std::memcmp (buffer, expected_buffer, 6), 0,
                         "Address encoding is incorrect");

  const auto newSatAddress = SatAddress::ConvertFrom (Address (0, buffer, 6));

  NS_TEST_ASSERT_MSG_EQ (newSatAddress, test, "New address is not identical to the old one");
  NS_TEST_ASSERT_MSG_EQ (newSatAddress.getConstellationId (), 0xf00f, "Wrong constellation id.");
  NS_TEST_ASSERT_MSG_EQ (newSatAddress.getOrbitalPlane (), 0xe11e, "Wrong orbital plane id.");
  NS_TEST_ASSERT_MSG_EQ (newSatAddress.getPlaneIndex (), 0xd22d, "Wrong plane index id.");

  std::istringstream s ("f00f:e11e:d22d");
  SatAddress test2;
  s >> test2;
  NS_TEST_ASSERT_MSG_EQ (test, test2, "Wrong conversion from istream");
}

// Add some help text to this case to describe what it is intended to test
SatAddressTestCase::SatAddressTestCase () : TestCase ("Check SatAddress conversion functions")
{
}

SatAddressTestCase::~SatAddressTestCase ()
{
}

class IcarusSatAddressTestSuite : public TestSuite
{
public:
  IcarusSatAddressTestSuite ();
};

IcarusSatAddressTestSuite::IcarusSatAddressTestSuite () : TestSuite ("icarus.sataddress", UNIT)
{
  // TestDuration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER
  AddTestCase (new SatAddressTestCase, TestCase::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
static IcarusSatAddressTestSuite icarusSatAddressTestSuite;
