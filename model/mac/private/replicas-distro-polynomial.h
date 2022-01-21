/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *
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
 *
 */

#ifndef REPLICAS_DISTRO_POLYNOMIAL_H
#define REPLICAS_DISTRO_POLYNOMIAL_H

#include "ns3/object.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {
namespace icarus {

class ReplicasDistroPolynomial : public Object
{
public:
  ReplicasDistroPolynomial (const std::vector<double> &c)
      : coefficients (c), rng (CreateObject<UniformRandomVariable> ())
  {
  }

  uint16_t
  NumReplicasPerPacket (void) const
  {
    double p = rng->GetValue ();
    uint16_t numReplicas = 0;
    double coeffSum = 0;
    for (auto n = 0u; n < coefficients.size (); n++)
      {
        coeffSum += coefficients[n];
        if (coefficients[n] > 0 && p < coeffSum)
          {
            numReplicas = n;
            break;
          }
      }

    NS_ASSERT_MSG (numReplicas > 0, "Invalid number of replicas per packet");

    return numReplicas;
  }

private:
  const std::vector<double> coefficients;
  Ptr<UniformRandomVariable> rng;
};
} // namespace icarus
} // namespace ns3

#endif