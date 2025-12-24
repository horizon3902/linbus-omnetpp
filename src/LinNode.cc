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
    scheduleRunning = true; // NEW (Phase 3): Schedule starts active

    scheduleTimer = new cMessage("scheduleTimer");
    headerTimer = new cMessage("headerTimer");
    responseTimeoutTimer = new cMessage("responseTimeoutTimer");
    wakeupTimer = new cMessage("wakeupTimer"); // NEW (Phase 3): External wakeup trigger

    parseSignals(par("signalConfiguration").stringValue());
    parseErrorSignals(par("errorSignalConfiguration").stringValue());

    if (isMaster) {
        EV_INFO << "Master node initialized.\n";

        // 1. Parse Normal Schedule
        parseSchedule(par("scheduleConfiguration").stringValue(), scheduleTable);

        // 2. Parse Collision Resolution Schedule (if exists)
        if (hasPar("collisionScheduleConfiguration")) {
            const char* colConf = par("collisionScheduleConfiguration").stringValue();
            if (colConf && strlen(colConf) > 0) {
                 parseSchedule(colConf, collisionScheduleTable);
                 EV_INFO << "Master configured with Collision Resolution Schedule.\n";
            }
        }

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
    // --- NEW (Phase 3): WAKEUP TIMER (Simulated External Event) ---
    if (msg == wakeupTimer) {
        EV_WARN << "External Wakeup Event! Sending Wakeup Pulse to Bus.\n";
        sendWakeupPulse();
        // The sender also wakes up immediately
        state = IDLE;
        if (isMaster) {
             scheduleRunning = true;
             if (!scheduleTimer->isScheduled()) scheduleAt(simTime() + 0.02, scheduleTimer);
        }
        return;
    }

    // --- SCHEDULE TIMER (MASTER) ---
    if (msg == scheduleTimer) {
        // NEW (Phase 3): Check if Schedule is Paused (Sleep Mode)
        if (!scheduleRunning) {
            EV_DETAIL << "Master is in Sleep Mode. Schedule paused.\n";
            return;
        }

        std::vector<LinSlot> *activeTable = resolvingCollision ? &collisionScheduleTable : &scheduleTable;
        int *idx = resolvingCollision ? &collisionSlotIndex : &currentSlotIndex;

        if (activeTable->empty()) return;

        LinSlot &slot = (*activeTable)[*idx];
        EV_INFO << ">>> Starting Slot for Frame ID 0x" << std::hex << slot.frameId << std::dec
                << (resolvingCollision ? " [RESOLVING]" : "") << endl;

        sendBreakSignal();
        double breakDuration = 13.0 / baudRate;
        scheduleAt(simTime() + breakDuration + interByteTime, headerTimer);

        // Schedule next slot
        scheduleAt(simTime() + slot.period, scheduleTimer);

        // Increment Index
        *idx = (*idx + 1);

        // Logic to switch back after resolution table finishes
        if (resolvingCollision && *idx >= (int)collisionScheduleTable.size()) {
            EV_INFO << "Collision Resolution Complete. Returning to Normal Schedule.\n";
            resolvingCollision = false;
            collisionSlotIndex = 0;
        } else {
             *idx = (*idx) % activeTable->size();
        }
        return;
    }

    if (msg == headerTimer) {
        std::vector<LinSlot> *activeTable = resolvingCollision ? &collisionScheduleTable : &scheduleTable;
        int *idx = resolvingCollision ? &collisionSlotIndex : &currentSlotIndex;

        int actualIdx = (*idx == 0) ? activeTable->size() - 1 : *idx - 1;
        LinSlot &slot = (*activeTable)[actualIdx];

        sendHeaderRemainder(slot.frameId, slot.publisher);

        if (isMaster && !isPublisherFor(slot.frameId)) {
            pendingResponseFrameId = slot.frameId;
            // Timeout calc
            double timeout = (20.0 + 90.0) / baudRate + interByteTime + (propDelay * 4);
            scheduleAt(simTime() + timeout, responseTimeoutTimer);
        }
        return;
    }

    // --- TIMEOUT TIMER ---
    if (msg == responseTimeoutTimer) {
        if (pendingResponseFrameId != -1) {
            EV_WARN << "Master TIMEOUT waiting for ID 0x" << std::hex << pendingResponseFrameId << std::dec << endl;
            errorState.lastTimeoutError = true;
            pendingResponseFrameId = -1;
        }
        return;
    }

    // --- NETWORK MESSAGES ---

    // NEW (Phase 3): Handle Sleep State Behavior
    // If in Sleep, ignore everything EXCEPT a valid Wakeup Pulse.
    if (state == SLEEP) {
        if (auto brk = dynamic_cast<LinBreakSignal*>(msg)) {
            if (isWakeupPulse(brk)) {
                EV_WARN << "WAKEUP PULSE DETECTED! Exiting Sleep Mode.\n";
                state = IDLE;
                if (isMaster) {
                    scheduleRunning = true;
                    if (!scheduleTimer->isScheduled()) {
                        scheduleAt(simTime() + 0.02, scheduleTimer);
                    }
                }
            } else {
                EV_DETAIL << "Ignored signal in Sleep (not a wakeup pulse).\n";
            }
        } else {
             EV_DETAIL << "Ignored non-break signal in Sleep.\n";
        }
        delete msg;
        return;
    }

    // --- Normal Operation Handling ---

    if (dynamic_cast<LinBreakSignal*>(msg)) {
        LinBreakSignal *brk = (LinBreakSignal*)msg;
        // Check if it's a Wakeup Pulse (even if awake, we reset)
        if (isWakeupPulse(brk)) {
             EV_DETAIL << "Received Wakeup Pulse while awake. Resetting.\n";
             state = IDLE;
             if (isMaster && !scheduleRunning) {
                  scheduleRunning = true;
                  if (!scheduleTimer->isScheduled()) scheduleAt(simTime() + 0.02, scheduleTimer);
             }
        } else {
             EV_DETAIL << "Received BREAK. Resetting state.\n";
             state = WAIT_FOR_SYNC;
        }
        delete msg;
        return;
    }

    if (dynamic_cast<LinHeader*>(msg)) {
        LinHeader *header = (LinHeader*)msg;

        // PID Check (Phase 1 Logic)
        int expectedPid = computePID(header->getFrameId());
        if (header->getPid() != expectedPid) {
            EV_WARN << "Header Error! PID mismatch for ID 0x" << std::hex << header->getFrameId() << std::dec << endl;
            errorState.lastHeaderError = true;
            delete msg;
            return;
        }

        processReceivedHeader(header);
        delete msg;
        return;
    }

    if (dynamic_cast<LinResponse*>(msg)) {
        LinResponse *rsp = (LinResponse*)msg;

        int pid = computePID(rsp->getFrameId());
        unsigned char expectedChecksum = computeChecksum(rsp, pid);

        // Check Collision Flag set by Bus (Phase 2)
        bool collision = rsp->getCollision();
        bool checksumError = (rsp->getChecksum() != (char)expectedChecksum);

        if (checksumError || collision) {
             EV_WARN << "Response Error (Checksum/Collision) for ID 0x" << std::hex << rsp->getFrameId() << std::dec << endl;
             errorState.lastChecksumError = true;

             // Master triggers Collision Resolution (Phase 2)
             if (isMaster && !resolvingCollision) {
                 if (!collisionScheduleTable.empty()) {
                     EV_WARN << "Collision detected! Switching to Resolution Schedule.\n";
                     startCollisionResolution();
                 }
             }
        } else {
             EV_INFO << "Received RESPONSE ID 0x" << std::hex << rsp->getFrameId()
                     << " Byte0=" << (int)rsp->getData(0) << std::dec << endl;

             // NEW (Phase 3): Check for Go-To-Sleep Command (ID 0x3C, Data[0] == 0x00)
             if (rsp->getFrameId() == 0x3C && rsp->getData(0) == 0x00) {
                 EV_WARN << "Received GO-TO-SLEEP Command. Entering Sleep Mode.\n";
                 state = SLEEP;
                 if (isMaster) {
                     scheduleRunning = false; // Stop the schedule
                     cancelEvent(scheduleTimer);
                 }
             }

             if (isMaster && rsp->getFrameId() == pendingResponseFrameId) {
                 cancelEvent(responseTimeoutTimer);
                 pendingResponseFrameId = -1;
             }
        }

        if (state != SLEEP) state = IDLE;
        delete msg;
        return;
    }

    delete msg;
}

// --- HELPERS ---

// NEW (Phase 3): Helper to detect long dominant pulse
bool LinNode::isWakeupPulse(LinBreakSignal *brk) {
    // Normal Break = 13 bits. Wakeup = > 20 bits (approx > 1ms at 19.2k)
    double duration = brk->getDuration();
    double bitTime = 1.0 / baudRate;
    return (duration > 20 * bitTime);
}

// NEW (Phase 3): Helper to send Wakeup Pulse
void LinNode::sendWakeupPulse() {
    LinBreakSignal *brk = new LinBreakSignal("LIN_Wakeup");
    // Send a long dominant pulse (e.g., 30 bits)
    brk->setDuration(30.0 / baudRate);
    send(brk, "gate$o");
}

void LinNode::startCollisionResolution() {
    resolvingCollision = true;
    collisionSlotIndex = 0;

    // Interrupt current schedule wait and start resolution immediately
    cancelEvent(scheduleTimer);
    scheduleAt(simTime() + 0.005, scheduleTimer);
}

void LinNode::parseSchedule(const char* config, std::vector<LinSlot>& table) {
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
            table.push_back(slot);
        }
    }
}

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
        }
    }
}

void LinNode::parseErrorSignals(const char* config) {
    if (!config || strlen(config) == 0) return;
    cStringTokenizer tokenizer(config, ";");
    while (tokenizer.hasMoreTokens()) {
        const char* token = tokenizer.nextToken();
        cStringTokenizer partTok(token, ":");
        std::vector<std::string> parts = partTok.asVector();
        if (parts.size() >= 3) {
            LinErrorSignalBinding bind;
            bind.frameId = (int)strtol(parts[0].c_str(), nullptr, 16);
            bind.byteOffset = atoi(parts[1].c_str());
            bind.bitIndex = atoi(parts[2].c_str());
            errorBindings.push_back(bind);
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

void LinNode::applyErrorBits(int frameId, LinResponse *rsp) {
    bool hasError = errorState.lastChecksumError || errorState.lastHeaderError || errorState.lastTimeoutError;
    if (!hasError) return;
    for (auto &bind : errorBindings) {
        if (bind.frameId == frameId) {
            char currentByte = rsp->getData(bind.byteOffset);
            currentByte |= (1 << bind.bitIndex);
            rsp->setData(bind.byteOffset, currentByte);
        }
    }
    errorState.lastChecksumError = false;
    errorState.lastHeaderError = false;
    errorState.lastTimeoutError = false;
}

void LinNode::sendResponse(int frameId, double initialDelay) {
    LinResponse *rsp = new LinResponse("LIN_Response");
    rsp->setFrameId(frameId);
    rsp->setDataLength(8);
    for(int i=0; i<8; i++) rsp->setData(i, 0);

    for (auto &sig : signalTable) {
        if (sig.frameId == frameId && sig.byteOffset < 8) {
            rsp->setData(sig.byteOffset, (char)sig.value);
        }
    }
    applyErrorBits(frameId, rsp);

    int pid = computePID(frameId);
    rsp->setChecksum(computeChecksum(rsp, pid));

    double responseTxTime = 90.0 / baudRate;
    sendDelayed(rsp, initialDelay + responseTxTime + propDelay, "gate$o");
    state = TRANSMITTING;
}

void LinNode::finish() {
    cancelAndDelete(scheduleTimer);
    cancelAndDelete(headerTimer);
    cancelAndDelete(responseTimeoutTimer);
    cancelAndDelete(wakeupTimer); // NEW
}
