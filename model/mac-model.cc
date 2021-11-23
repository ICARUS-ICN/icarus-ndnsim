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
 * Author: Sergio Herrería Alonso <sha@det.uvigo.es>
 */

#include "mac-model.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"

namespace ns3 {
namespace icarus {

  NS_LOG_COMPONENT_DEFINE ("icarus.MacModel");
    
  NS_OBJECT_ENSURE_REGISTERED (MacModel);
    
  TypeId
  MacModel::GetTypeId (void)
  {
    static TypeId tid =
      TypeId ("ns3::icarus::MacModel")
      .SetParent<Object> ()
      .SetGroupName ("ICARUS");
    
    return tid;
  }

  MacModel::MacModel ()
  {
    NS_LOG_FUNCTION (this);
  }
  
  MacModel::~MacModel ()
  {
    NS_LOG_FUNCTION (this);
  }

}
}
