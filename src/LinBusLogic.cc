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

#include "LinBusLogic.h"
#include "LinDataFrame_m.h"

Define_Module(LinBusLogic);

void LinBusLogic::initialize() {
    EV << "LIN bus initialized\n";
}

void LinBusLogic::handleMessage(cMessage *msg) {
    auto *frame = check_and_cast<LinDataFrame *>(msg);
    EV << "Bus forwarding frame ID " << frame->getFrameId() << endl;

    for (int i = 0; i < gateSize("gate$o"); i++)
        send(frame->dup(), "gate$o", i);

    delete frame;
}
