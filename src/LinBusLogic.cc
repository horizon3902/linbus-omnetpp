#include "LinBusLogic.h"
#include "LinDataFrame_m.h" // Needed to cast to specific types
#include <omnetpp.h>

Define_Module(LinBusLogic);

void LinBusLogic::initialize() {
    EV_INFO << "LIN bus initialized (Physical Layer Forwarder)" << endl;

    // 1. Read Timing Parameters (used for collision duration calc)
    baudRate = par("baudRate");
    propDelay = par("propDelay");
    interByteTime = par("interByteTime");

    // 2. Read Error Injection Parameters (Phase 1)
    frameErrorRate = par("frameErrorRate");
    frameDropRate = par("frameDropRate");

    // 3. NEW (Phase 2): Collision Tracking Init
    isResponseOnBus = false;
    transmissionEndTimer = new cMessage("txEnd");
}

void LinBusLogic::handleMessage(cMessage *msg) {
    // --- 1. HANDLE SELF-MESSAGES (Collision Timer) ---
    // If this is our own timer saying "The previous frame finished", reset the busy flag.
    if (msg == transmissionEndTimer) {
        EV_DETAIL << "Bus: Transmission ended. Bus is now free." << endl;
        isResponseOnBus = false;
        return; // It's a self-message, not a frame to forward
    }

    // --- 2. CAST TO PACKET ---
    cPacket *frame = check_and_cast<cPacket *>(msg);

    // --- 3. ERROR INJECTION LOGIC (Phase 1 - Retained) ---

    // A. Frame Drop
    if (uniform(0, 1) < frameDropRate) {
        EV_WARN << "Bus DROPPING frame: " << frame->getName() << endl;
        delete frame;
        return;
    }

    // B. Frame Corruption
    if (uniform(0, 1) < frameErrorRate) {
        EV_WARN << "Bus CORRUPTING frame: " << frame->getName() << endl;

        if (auto rsp = dynamic_cast<LinResponse*>(frame)) {
            int len = rsp->getDataLength();
            if (len > 0) {
                int idx = intuniform(0, len - 1);
                char oldVal = rsp->getData(idx);
                rsp->setData(idx, oldVal ^ 0xFF); // Flip bits
            }
        } else if (auto hdr = dynamic_cast<LinHeader*>(frame)) {
            hdr->setPid(hdr->getPid() ^ 0xFF); // Flip PID
        }
    }

    // --- 4. NEW (Phase 2): COLLISION DETECTION ---

    // We only care about collisions for RESPONSES (multiple slaves replying to one header)
    if (LinResponse *rsp = dynamic_cast<LinResponse*>(frame)) {

        if (isResponseOnBus) {
            // Case A: Bus is already busy with another response -> COLLISION!
            EV_WARN << "COLLISION DETECTED on Bus for ID 0x" << std::hex << rsp->getFrameId() << std::dec << endl;

            // Mark this frame as collided.
            // In reality, BOTH frames get corrupted.
            // In simulation, marking the second one is enough for the Master to see "Collision".
            rsp->setCollision(true);
            rsp->setName("LIN_COLLISION");

        } else {
            // Case B: Bus is free -> Start tracking this transmission
            isResponseOnBus = true;

            // Calculate how long this frame occupies the bus
            // Formula: (DataBytes + Checksum) * 10 bits/byte (Start + 8 Data + Stop)
            int totalBits = (rsp->getDataLength() + 1) * 10;
            double txDuration = (double)totalBits / baudRate;

            // Schedule the "Free the bus" timer
            scheduleAt(simTime() + txDuration, transmissionEndTimer);
        }
    }

    // --- 5. FORWARDING (Retained) ---
    EV_INFO << "Bus forwarding signal: " << frame->getName()
            << " (" << frame->getBitLength() << " bits)" << endl;

    int gateCount = gateSize("gate$o");
    for (int i = 0; i < gateCount; i++) {
        // Standard OMNeT++ idiom: dup() for N-1 ports, send original to last port
        send(i == gateCount - 1 ? frame : frame->dup(), "gate$o", i);
    }
}
