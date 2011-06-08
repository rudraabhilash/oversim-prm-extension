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

package oversim.overlay.prm;

import oversim.overlay.nice.Nice;
//import oversim.common.BaseOverlay;
import oversim.common.IOverlay;

@namespace(oversim);

simple Prm extends Nice {
	parameters:
		@class(Prm);
}

module PrmModules like IOverlay {
    parameters:
        @display("i=block/network2");

    gates:
        input udpIn;   // gate from the UDP layer
        output udpOut;    // gate to the UDP layer
        input tcpIn;    // gate from the TCP layer
        output tcpOut;    // gate to the TCP layer
        input appIn;   // gate from the application
        output appOut;    // gate to the application
            
    submodules:
        prm: Prm {
            parameters:
                @display("p=60,60");

        }
    connections allowunconnected:
        udpIn --> prm.udpIn;
        udpOut <-- prm.udpOut;
        appIn --> prm.appIn;
        appOut <-- prm.appOut;    
}


