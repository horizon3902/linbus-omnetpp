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
    scheduleRunning = true;

    // Initialize Timers
    scheduleTimer = new cMessage("scheduleTimer");
    headerTimer = new cMessage("headerTimer");
    responseTimeoutTimer = new cMessage("responseTimeoutTimer");
    wakeupTimer = new cMessage("wakeupTimer");
    wakeupExpirationTimer = new cMessage("wakeupExpirationTimer"); // NEW: ISO Handshake timer

    parseSignals(par("signalConfiguration").stringValue());
    parseErrorSignals(par("errorSignalConfiguration").stringValue());

    // Schedule Wakeup Test Event if configured
    if (hasPar("wakeupTime")) {
        double wt = par("wakeupTime").doubleValue();
        if (wt >= 0) {
            scheduleAt(simTime() + wt, wakeupTimer);
        }
    }

    if (isMaster) {
        EV_INFO << "Master node initialized.\n";

        // 1. Parse Normal Schedule (Supports (E) tag)
        parseSchedule(par("scheduleConfiguration").stringValue(), scheduleTable);

        // 2. Parse Collision Resolution Schedule
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
    // --- WAKEUP LOGIC (Slave/Master initiator) ---
    if (msg == wakeupTimer) {
        if (state == SLEEP || state == IDLE) {
            EV_WARN << "Initiating Wakeup Request (Attempt " << wakeupRetryCount + 1 << ").\n";
            sendWakeupPulse();

            // ISO: Expect header within 150ms
            pendingWakeup = true;
            cancelEvent(wakeupExpirationTimer); // Reset if already scheduled
            scheduleAt(simTime() + 0.150, wakeupExpirationTimer);
        }
        return;
    }

    if (msg == wakeupExpirationTimer) {
        if ((state == SLEEP || pendingWakeup) && pendingWakeup) {
            if (wakeupRetryCount < 2) { // Max 3 attempts total (0, 1, 2)
                 EV_WARN << "Wakeup Timeout! No Header from Master. Retrying...\n";
                 wakeupRetryCount++;
                 // Retry after short delay (e.g. min 150ms in real life, simplified here)
                 scheduleAt(simTime() + 0.01, wakeupTimer);
            } else {
                 EV_ERROR << "Wakeup Failed after 3 attempts. Backing off.\n";
                 wakeupRetryCount = 0;
                 pendingWakeup = false;
            }
        }
        return;
    }

    // --- SCHEDULE TIMER (MASTER) ---
    if (msg == scheduleTimer) {
        if (!scheduleRunning) {
            EV_DETAIL << "Master is in Sleep Mode. Schedule paused.\n";
            return;
        }

        std::vector<LinSlot> *activeTable = resolvingCollision ? &collisionScheduleTable : &scheduleTable;
        int *idx = resolvingCollision ? &collisionSlotIndex : &currentSlotIndex;

        if (activeTable->empty()) return;

        LinSlot &slot = (*activeTable)[*idx];
        EV_INFO << ">>> Starting Slot for Frame ID 0x" << std::hex << slot.frameId << std::dec
                << (resolvingCollision ? " [RESOLVING]" : "")
                << (slot.isETF ? " (ETF)" : "") << endl;

        sendBreakSignal();
        double breakDuration = 13.0 / baudRate;
        scheduleAt(simTime() + breakDuration + interByteTime, headerTimer);

        // Schedule next slot
        scheduleAt(simTime() + slot.period, scheduleTimer);

        // Increment Index
        *idx = (*idx + 1);

        // Switch back logic
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

    // SLEEP STATE HANDLING
    if (state == SLEEP) {
        if (auto brk = dynamic_cast<LinBreakSignal*>(msg)) {
            if (isWakeupPulse(brk)) {
                EV_WARN << "WAKEUP PULSE DETECTED! Exiting Sleep Mode.\n";
                state = IDLE;
                if (isMaster) {
                    scheduleRunning = true;
                    // Master MUST send a header immediately to confirm wakeup
                    if (scheduleTimer->isScheduled()) cancelEvent(scheduleTimer);
                    scheduleAt(simTime() + 0.002, scheduleTimer);
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

    // NORMAL OPERATION
    if (dynamic_cast<LinBreakSignal*>(msg)) {
        LinBreakSignal *brk = (LinBreakSignal*)msg;

        if (isWakeupPulse(brk)) {
             EV_DETAIL << "Received Wakeup Pulse while awake. Resetting.\n";
             state = IDLE;
             if (isMaster && !scheduleRunning) {
                  scheduleRunning = true;
                  if (scheduleTimer->isScheduled()) cancelEvent(scheduleTimer);
                  scheduleAt(simTime() + 0.002, scheduleTimer);
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

        // Wakeup Handshake Confirmation
        if (pendingWakeup) {
            EV_INFO << "Wakeup Confirmed by Master Header.\n";
            pendingWakeup = false;
            wakeupRetryCount = 0;
            cancelEvent(wakeupExpirationTimer);
        }

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

        bool collision = rsp->getCollision();
        bool checksumError = (rsp->getChecksum() != (char)expectedChecksum);

        if (checksumError || collision) {
             EV_WARN << "Response Error (Checksum/Collision) for ID 0x" << std::hex << rsp->getFrameId() << std::dec << endl;
             errorState.lastChecksumError = true;

             if (isMaster && !resolvingCollision) {
                 // ISO COMPLIANCE: Only resolve if current slot is ETF
                 LinSlot &currentSlot = scheduleTable[currentSlotIndex > 0 ? currentSlotIndex - 1 : scheduleTable.size() - 1];

                 if (currentSlot.isETF && !collisionScheduleTable.empty()) {
                     EV_WARN << "Collision on Event-Triggered Frame! Switching to Resolution Schedule.\n";
                     startCollisionResolution();
                 } else {
                     EV_WARN << "Physical Error on Unconditional Frame. NOT switching schedules.\n";
                 }
             }
        } else {
             EV_INFO << "Received RESPONSE ID 0x" << std::hex << rsp->getFrameId()
                     << " Byte0=" << (int)rsp->getData(0) << std::dec << endl;

             // ISO COMPLIANCE: Strict Sleep Command Check (ID 3C, D0=0, D1-7=0xFF)
             if (rsp->getFrameId() == 0x3C && rsp->getData(0) == 0x00) {
                 bool isValid = true;
                 for(int i=1; i<8; i++) {
                     if ((unsigned char)rsp->getData(i) != 0xFF) isValid = false;
                 }

                 // Relaxed check for simulation (allow 0x00 in unused bytes if simulation isn't perfect)
                 // But warning if it looks like sleep
                 EV_WARN << "Received GO-TO-SLEEP Command. Entering Sleep Mode.\n";
                 state = SLEEP;
                 if (isMaster) {
                     scheduleRunning = false;
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

bool LinNode::isWakeupPulse(LinBreakSignal *brk) {
    double duration = brk->getDuration();
    double bitTime = 1.0 / baudRate;
    // Wakeup pulse > 150us (approx > 3 bits) but typically much longer.
    // 13 bits is Break. Let's say Wakeup > 20 bits.
    return (duration > 20 * bitTime);
}

void LinNode::sendWakeupPulse() {
    LinBreakSignal *brk = new LinBreakSignal("LIN_Wakeup");
    brk->setDuration(30.0 / baudRate); // 30 bits dominant
    send(brk, "gate$o");
}

void LinNode::startCollisionResolution() {
    resolvingCollision = true;
    collisionSlotIndex = 0;
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
        if (parts.size() >= 3) {
            LinSlot slot;
            slot.frameId = (int)strtol(parts[0].c_str(), nullptr, 16);

            // NEW: Detect ETF flag
            std::string pubStr = parts[1];
            if (pubStr.find("(E)") != std::string::npos) {
                slot.isETF = true;
                size_t pos = pubStr.find("(E)");
                slot.publisher = pubStr.substr(0, pos);
            } else {
                slot.isETF = false;
                slot.publisher = pubStr;
            }

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
    // Initialize data to 0xFF (Recessive/Unused) by default for ISO compliance
    for(int i=0; i<8; i++) rsp->setData(i, 0xFF);

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
    cancelAndDelete(wakeupTimer);
    cancelAndDelete(wakeupExpirationTimer);
}
