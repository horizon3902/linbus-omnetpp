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
    EV_INFO << "Initializing LIN bus with schedule table" << endl;

    // Master triggers these frames periodically
    scheduleTable.push_back({0x10, "master", 0.01});
    scheduleTable.push_back({0x12, "slave1", 0.01});
    scheduleTable.push_back({0x14, "slave2", 0.01});

    slotTimer = new cMessage("slotTimer");
    scheduleAt(simTime() + scheduleTable[0].period, slotTimer);
}

void LinBusLogic::handleMessage(cMessage *msg) {
    if (msg == slotTimer) {
        LinSlot &slot = scheduleTable[currentSlotIndex];
        EV_INFO << "Triggering slot for Frame ID " << slot.frameId
                << " (publisher: " << slot.publisher << ")" << endl;

        LinDataFrame *header = new LinDataFrame("LIN_Header");
        header->setFrameId(slot.frameId);
        header->setIsResponse(false);
        header->setDataLength(0);

        // Broadcast the header
        for (int i = 0; i < gateSize("gate$o"); i++) {
            send(i == gateSize("gate$o") - 1 ? header : header->dup(), "gate$o", i);
        }


        // Move to next slot
        currentSlotIndex = (currentSlotIndex + 1) % scheduleTable.size();
        scheduleAt(simTime() + scheduleTable[currentSlotIndex].period, slotTimer);
    }
    else {
        // Forward all received frames to all nodes (bus broadcast)
        LinDataFrame *frame = check_and_cast<LinDataFrame *>(msg);
        EV_INFO << "Bus forwarding frame ID " << frame->getFrameId() << endl;
        for (int i = 0; i < gateSize("gate$o"); i++) {
            send(i == gateSize("gate$o") - 1 ? frame : frame->dup(), "gate$o", i);
        }


    }
}


