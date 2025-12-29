#include "LinBusLogic.h"
#include "LinDataFrame_m.h"
#include <omnetpp.h>

Define_Module(LinBusLogic);

void LinBusLogic::initialize() {
    EV_INFO << "LIN bus initialized (Timed forwarder + collision model)" << endl;

    baudRate = par("baudRate");
    propDelay = par("propDelay");
    interByteTime = par("interByteTime");

    frameErrorRate = par("frameErrorRate");
    frameDropRate  = par("frameDropRate");

    isResponseOnBus = false;
    transmissionEndTimer = new cMessage("txEnd");

    pendingResponse = nullptr;
}

void LinBusLogic::finish() {
    cancelAndDelete(transmissionEndTimer);
    transmissionEndTimer = nullptr;

    if (pendingResponse) {
        delete pendingResponse;
        pendingResponse = nullptr;
    }
}

double LinBusLogic::headerRemainderTxTime() const {
    // Sync + PID = 2 bytes; 10 bits/byte (start + 8 data + stop)
    const int bits = 2 * 10;
    return (double)bits / baudRate;
}

double LinBusLogic::responseTxTime(int dataLength) const {
    int n = dataLength;
    if (n < 0) n = 0;
    if (n > 8) n = 8; // keep model bounded
    const int bytes = n + 1; // + checksum
    const int bits = bytes * 10;
    return (double)bits / baudRate;
}

void LinBusLogic::forwardToAll(cPacket *frame, simtime_t extraDelay) {
    int gateCount = gateSize("gate$o");
    for (int i = 0; i < gateCount; i++) {
        cPacket *out = (i == gateCount - 1) ? frame : frame->dup();
        sendDelayed(out, extraDelay, "gate$o", i);
    }
}

void LinBusLogic::handleMessage(cMessage *msg) {
    // --- 1) SELF-MESSAGE: end of response window ---
    if (msg == transmissionEndTimer) {
        EV_DETAIL << "Bus: Response window ended. Bus is now free." << endl;
        isResponseOnBus = false;

        if (pendingResponse) {
            EV_INFO << "Bus forwarding signal: " << pendingResponse->getName()
                    << " (" << pendingResponse->getBitLength() << " bits)" << endl;

            // Deliver at end-of-response + propagation
            forwardToAll(pendingResponse, propDelay);
            pendingResponse = nullptr;
        }
        return;
    }

    // --- 2) Determine which port injected the signal ---
    int srcGateIndex = -1;
    if (msg->getArrivalGate() != nullptr) {
        srcGateIndex = msg->getArrivalGate()->getIndex();
    }

    // --- 3) Cast to packet ---
    cPacket *frame = check_and_cast<cPacket *>(msg);

    // --- 4) Stamp source address (for echo filtering in nodes) ---
    if (auto brk = dynamic_cast<LinBreakSignal*>(frame)) {
        brk->setSourceAddr(srcGateIndex);
    } else if (auto hdr = dynamic_cast<LinHeader*>(frame)) {
        hdr->setSourceAddr(srcGateIndex);
    } else if (auto rsp = dynamic_cast<LinResponse*>(frame)) {
        rsp->setSourceAddr(srcGateIndex);
    }

    // --- 5) Error injection: drop ---
    if (uniform(0, 1) < frameDropRate) {
        EV_WARN << "Bus DROPPING frame: " << frame->getName() << endl;
        delete frame;
        return;
    }

    // --- 6) Error injection: corrupt ---
    if (uniform(0, 1) < frameErrorRate) {
        EV_WARN << "Bus CORRUPTING frame: " << frame->getName() << endl;

        if (auto rsp = dynamic_cast<LinResponse*>(frame)) {
            int len = rsp->getDataLength();
            if (len > 0) {
                int idx = intuniform(0, len - 1);
                char oldVal = rsp->getData(idx);
                rsp->setData(idx, oldVal ^ 0xFF);
            }
        } else if (auto hdr = dynamic_cast<LinHeader*>(frame)) {
            hdr->setPid(hdr->getPid() ^ 0xFF);
        }
    }

    // --- 7) RESPONSE handling: collision + timed delivery at end-of-response ---
    if (auto rsp = dynamic_cast<LinResponse*>(frame)) {

        if (!isResponseOnBus) {
            isResponseOnBus = true;
            pendingResponse = rsp;

            double txDuration = responseTxTime(rsp->getDataLength());
            scheduleAt(simTime() + txDuration, transmissionEndTimer);

            // Do not forward now (receiver sees frame at end-of-response)
            return;
        }

        // Collision: invalidate the response slot
        EV_WARN << "COLLISION DETECTED on Bus for ID 0x" << std::hex << rsp->getFrameId() << std::dec << endl;

        if (pendingResponse) {
            pendingResponse->setCollision(true);
            pendingResponse->setName("LIN_COLLISION");
        }

        delete rsp;
        return;
    }

    // --- 8) HEADER handling: timed delivery at end-of-header remainder ---
    if (auto hdr = dynamic_cast<LinHeader*>(frame)) {
        EV_INFO << "Bus forwarding signal: " << hdr->getName()
                << " (" << hdr->getBitLength() << " bits)" << endl;

        double tx = headerRemainderTxTime();
        forwardToAll(hdr, tx + propDelay);
        return;
    }

    // --- 9) BREAK / others: forward with propagation only ---
    EV_INFO << "Bus forwarding signal: " << frame->getName()
            << " (" << frame->getBitLength() << " bits)" << endl;

    forwardToAll(frame, propDelay);
}
