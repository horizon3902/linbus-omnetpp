#include "LinNode.h"
#include "LinUtil.h"
#include <stdlib.h>

Define_Module(LinNode);

void LinNode::initialize() {
    isMaster = par("isMaster");
    baudRate = par("baudRate");
    propDelay = par("propDelay");
    interByteTime = par("interByteTime");

    state = IDLE;
    scheduleTimer = new cMessage("scheduleTimer");
    headerTimer = new cMessage("headerTimer");

    // 1. Parse Signal Configuration (Who am I?)
    parseSignals(par("signalConfiguration").stringValue());

    // 2. Parse Schedule (If Master)
    if (isMaster) {
        EV_INFO << "Master node initialized.\n";
        parseSchedule(par("scheduleConfiguration").stringValue());

        if (!scheduleTable.empty()) {
             scheduleAt(simTime() + scheduleTable[0].period, scheduleTimer);
        } else {
             EV_WARN << "Master has no schedule!\n";
        }
    } else {
        EV_INFO << "Slave node initialized.\n";
    }
}

void LinNode::handleMessage(cMessage *msg) {
    // --- SELF MESSAGES ---
    if (msg == scheduleTimer) {
        if (scheduleTable.empty()) return;
        LinSlot &slot = scheduleTable[currentSlotIndex];
        EV_INFO << ">>> Starting Slot for Frame ID 0x" << std::hex << slot.frameId << std::dec << endl;

        sendBreakSignal();

        double breakDuration = 13.0 / baudRate;
        scheduleAt(simTime() + breakDuration + interByteTime, headerTimer);

        scheduleAt(simTime() + slot.period, scheduleTimer);
        currentSlotIndex = (currentSlotIndex + 1) % scheduleTable.size();
        return;
    }

    if (msg == headerTimer) {
        int idx = (currentSlotIndex == 0) ? scheduleTable.size() - 1 : currentSlotIndex - 1;
        int frameId = scheduleTable[idx].frameId;
        std::string publisher = scheduleTable[idx].publisher; // Cosmetic name only now

        sendHeaderRemainder(frameId, publisher);
        return;
    }

    // --- NETWORK MESSAGES ---
    if (dynamic_cast<LinBreakSignal*>(msg)) {
        EV_DETAIL << "Received BREAK. Resetting state.\n";
        state = WAIT_FOR_SYNC;
        delete msg;
        return;
    }

    if (dynamic_cast<LinHeader*>(msg)) {
        LinHeader *header = (LinHeader*)msg;
        if (state != WAIT_FOR_SYNC && state != IDLE) {
             EV_WARN << "Header received out of sync! State=" << state << endl;
        }
        processReceivedHeader(header);
        delete msg;
        return;
    }

    if (dynamic_cast<LinResponse*>(msg)) {
        LinResponse *rsp = (LinResponse*)msg;
        EV_INFO << "Received RESPONSE ID 0x" << std::hex << rsp->getFrameId()
                << " Byte0=" << (int)rsp->getData(0) << std::dec << endl;
        state = IDLE;
        delete msg;
        return;
    }

    delete msg;
}

// --- HELPERS ---

void LinNode::parseSignals(const char* config) {
    if (!config || strlen(config) == 0) return;

    cStringTokenizer tokenizer(config, ";");
    while (tokenizer.hasMoreTokens()) {
        const char* token = tokenizer.nextToken();
        cStringTokenizer partTok(token, ":");
        std::vector<std::string> parts = partTok.asVector();

        if (parts.size() >= 3) {
            LinSignal sig;
            sig.frameId = (int)strtol(parts[0].c_str(), nullptr, 16);
            sig.byteOffset = atoi(parts[1].c_str());
            sig.value = atoi(parts[2].c_str());
            signalTable.push_back(sig);
            EV_DETAIL << "Configured Signal: ID=0x" << std::hex << sig.frameId
                      << " Byte=" << std::dec << sig.byteOffset
                      << " Val=" << sig.value << endl;
        }
    }
}

void LinNode::parseSchedule(const char* config) {
    if (!config || strlen(config) == 0) return;
    cStringTokenizer tokenizer(config, ";");
    while (tokenizer.hasMoreTokens()) {
        const char* token = tokenizer.nextToken();
        cStringTokenizer entryTok(token, ":");
        std::vector<std::string> parts = entryTok.asVector();
        if (parts.size() == 3) {
            LinSlot slot;
            slot.frameId = (int)strtol(parts[0].c_str(), nullptr, 16);
            slot.publisher = parts[1];
            slot.period = atof(parts[2].c_str());
            scheduleTable.push_back(slot);
        }
    }
}

bool LinNode::isPublisherFor(int frameId) {
    for (auto &sig : signalTable) {
        if (sig.frameId == frameId) return true;
    }
    return false;
}

void LinNode::sendBreakSignal() {
    LinBreakSignal *brk = new LinBreakSignal("LIN_Break");
    brk->setDuration(13.0 / baudRate);
    send(brk, "gate$o");
}

void LinNode::sendHeaderRemainder(int frameId, std::string publisher) {
    LinHeader *header = new LinHeader("LIN_Header");
    header->setFrameId(frameId);
    header->setPid(computePID(frameId));
    header->setSyncByte(0x55);

    double headerTxTime = 20.0 / baudRate;
    sendDelayed(header, headerTxTime + propDelay, "gate$o");

    // Master Logic: Check if I am the publisher based on my own signal table
    if (isPublisherFor(frameId)) {
        double delay = headerTxTime + interByteTime;
        sendResponse(frameId, delay);
    }
}

void LinNode::processReceivedHeader(LinHeader *header) {
    state = WAIT_FOR_PID;
    int id = header->getFrameId();

    if (isPublisherFor(id)) {
        EV_INFO << "I am publisher for ID 0x" << std::hex << id << endl;
        double delay = interByteTime;
        sendResponse(id, delay);
    } else {
        state = RECEIVING;
    }
}

void LinNode::sendResponse(int frameId, double initialDelay) {
    LinResponse *rsp = new LinResponse("LIN_Response");
    rsp->setFrameId(frameId);
    rsp->setDataLength(8);

    // Default fill 0
    for(int i=0; i<8; i++) rsp->setData(i, 0);

    // Fill from Signal Table
    for (auto &sig : signalTable) {
        if (sig.frameId == frameId && sig.byteOffset < 8) {
            rsp->setData(sig.byteOffset, (char)sig.value);
        }
    }

    int pid = computePID(frameId);
    rsp->setChecksum(computeChecksum(rsp, pid));

    double responseTxTime = 90.0 / baudRate;
    sendDelayed(rsp, initialDelay + responseTxTime + propDelay, "gate$o");
    state = TRANSMITTING;
}

void LinNode::finish() {
    cancelAndDelete(scheduleTimer);
    cancelAndDelete(headerTimer);
}
