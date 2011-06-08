//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

cplusplus {{
#include "NiceMessage_m.h"

#define PRMRANDOMWALK_L(msg) (NICEMESSAGE_L(msg) + INT_L)
#define PRMMULTICAST_L(msg) (NICEMULTICAST_L(msg) + (msg->getPkts_seqNoArraySize() * INT_L))

}}
class NiceMessage;
class NiceMulticastMessage;

enum PrmCommand {
	PRM_MULTICAST = 18;
	PRM_NAK_MULTICAST = 19;
	PRM_RANDOM_WALK = 20;
	PRM_RANDOM_WALK_RESPONSE = 21;   
}

message PrmMulticastMessage extends NiceMulticastMessage {
 	long pkts_seqNo[];    
}

//
// TODO generated message class
//
message PrmRandomWalkMessage extends NiceMessage {
    int ttl;
}
