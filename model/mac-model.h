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
 * Author: Sergio Herrería Alonso <sha@det.uvigo.es>
 *
 */

#ifndef MAC_MODEL_H
#define MAC_MODEL_H

#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"

namespace ns3 {
namespace icarus {
    
  class MacModel : public Object
  {
  public:
    static TypeId GetTypeId (void);
    MacModel ();
    virtual ~MacModel ();
    
    enum MacProtocol {
		      NONE,
		      ALOHA
    };
    virtual MacProtocol GetMacProtocol (void) const = 0;
    
    virtual void NewPacketRx (const Ptr<Packet> &packet, Time packet_tx_time) = 0;
    virtual bool HasCollided (const Ptr<Packet> &packet) = 0;
  };
    
}
}

#endif
