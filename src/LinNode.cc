#include "LinNode.h"
#include "LinUtil.h"

#include <stdlib.h>

Define_Module(LinNode);

void LinNode::initialize() {
    isMaster = par("isMaster");
    nodeAddress = par("nodeAddress");

    baudRate = par("baudRate");
    propDelay = par("propDelay");
    interByteTime = par("interByteTime");

    breakBits = par("breakBits").intValue();
    delimiterBits = par("delimiterBits").intValue();
    if (breakBits < 1) breakBits = 13;
    if (delimiterBits < 0) delimiterBits = 1;

    state = IDLE;
    scheduleRunning = true;

    // --------------------
    // Diagnostics (Phase 2)
    // --------------------
    diagCtx.enabled = par("enableDiagnostics").boolValue();;
    diagCtx.role = DIAG_NONE;
    diagCtx.tpState = DIAG_TP_IDLE;
    diagCtx.rxBuffer.clear();
    diagCtx.txBuffer.clear();

    if (isMaster) {
        diagCtx.role = DIAG_MASTER;
    } else {
        diagCtx.role = DIAG_SLAVE;
    }

    // --------------------

    scheduleTimer = new cMessage("scheduleTimer");
    headerTimer = new cMessage("headerTimer");
    responseTimeoutTimer = new cMessage("responseTimeoutTimer");
    wakeupTimer = new cMessage("wakeupTimer");
    wakeupExpirationTimer = new cMessage("wakeupExpirationTimer");
    goToSleepTimer = new cMessage("goToSleepTimer");

    eventRearmTimer = new cMessage("eventRearmTimer");
    eventRearmPeriod = par("eventRearmPeriod").doubleValue();
    eventProbability = par("eventProbability").doubleValue();

    inactivityTimer = new cMessage("inactivityTimer");
    inactivitySleepTime = par("inactivitySleepTime").doubleValue();
    resetInactivityTimer();

    resetRxContext();

    parseSignals(par("signalConfiguration").stringValue());
    parseErrorSignals(par("errorSignalConfiguration").stringValue());

    // DLC map
    parseFrameDlcs(par("frameDlcConfiguration").stringValue());

    // ETF mapping + event flags
    parseEventFrames(par("eventFrameConfiguration").stringValue());
    initializeEventFlags();
    if (eventRearmPeriod >= 0) {
        scheduleAt(simTime() + eventRearmPeriod, eventRearmTimer);
    }

    if (hasPar("wakeupTime")) {
        double wt = par("wakeupTime").doubleValue();
        if (wt >= 0) {
            scheduleAt(simTime() + wt, wakeupTimer);
        }
    }

    if (isMaster) {
        EV_INFO << "Master node initialized.\n";

        parseSchedule(par("scheduleConfiguration").stringValue(), scheduleTable);

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

double LinNode::headerRemainderTxTime() const {
    return (double)(2 * 10) / baudRate;
}

double LinNode::responseTxTime(int dataLength) const {
    int n = dataLength;
    if (n < 0) n = 0;
    if (n > 8) n = 8;
    return (double)((n + 1) * 10) / baudRate;
}

void LinNode::resetRxContext() {
    rxSawBreak = false;
    rxAwaitingResponse = false;
    rxExpectedResponseId = -1;
}

void LinNode::parseFrameDlcs(const char* config) {
    frameIdToDlc.clear();
    if (!config || strlen(config) == 0) return;

    // Format: "10=2;21=4;22=4;3C=8"
    cStringTokenizer tokenizer(config, ";");
    while (tokenizer.hasMoreTokens()) {
        const char* token = tokenizer.nextToken();
        cStringTokenizer kvTok(token, "=");
        std::vector<std::string> kv = kvTok.asVector();
        if (kv.size() != 2) continue;

        int frameId = (int)strtol(kv[0].c_str(), nullptr, 16);
        int dlc = atoi(kv[1].c_str());
        if (dlc < 0) dlc = 0;
        if (dlc > 8) dlc = 8;

        frameIdToDlc[frameId] = dlc;
    }
}

int LinNode::dlcForFrame(int frameId) const {
    auto it = frameIdToDlc.find(frameId);
    if (it != frameIdToDlc.end()) return it->second;
    return 8; // default
}

bool LinNode::isGoToSleepCommand(const LinResponse *rsp) const {
    if (!rsp) return false;
    if (rsp->getFrameId() != 0x3C) return false;
    if ((unsigned char)rsp->getData(0) != 0x00) return false;

    for (int i = 1; i < rsp->getDataLength(); i++) {
        if ((unsigned char)rsp->getData(i) != 0xFF) return false;
    }
    // For a strict LIN 2.x GOTO-SLEEP, DLC should be 8; keep model tolerant.
    return true;
}

// -------------------- ETF helpers --------------------
void LinNode::parseEventFrames(const char* config) {
    etfToUcfs.clear();
    if (!config || strlen(config) == 0) return;

    cStringTokenizer tokenizer(config, ";");
    while (tokenizer.hasMoreTokens()) {
        const char* token = tokenizer.nextToken();  // "20:21,22"
        cStringTokenizer kvTok(token, ":");
        std::vector<std::string> kv = kvTok.asVector();
        if (kv.size() != 2) continue;

        int etfId = (int)strtol(kv[0].c_str(), nullptr, 16);
        std::vector<int> ucfs;

        cStringTokenizer listTok(kv[1].c_str(), ",");
        while (listTok.hasMoreTokens()) {
            const char* idStr = listTok.nextToken();
            int ucfId = (int)strtol(idStr, nullptr, 16);
            ucfs.push_back(ucfId);
        }

        if (!ucfs.empty()) {
            etfToUcfs[etfId] = ucfs;
        }
    }
}

void LinNode::initializeEventFlags() {
    ucfEventPending.clear();
    for (auto &sig : signalTable) {
        if (ucfEventPending.find(sig.frameId) == ucfEventPending.end()) {
            ucfEventPending[sig.frameId] = true;
        }
    }
}

bool LinNode::isEventFrameId(int etfId) const {
    return (etfToUcfs.find(etfId) != etfToUcfs.end());
}

bool LinNode::selectUcfForEventFrame(int etfId, int& outUcfId) const {
    auto it = etfToUcfs.find(etfId);
    if (it == etfToUcfs.end()) return false;

    const std::vector<int>& candidates = it->second;
    for (int ucfId : candidates) {
        auto pendIt = ucfEventPending.find(ucfId);
        bool pending = (pendIt != ucfEventPending.end()) ? pendIt->second : false;
        if (pending) {
            bool publishes = false;
            for (auto &sig : signalTable) {
                if (sig.frameId == ucfId) { publishes = true; break; }
            }
            if (publishes) {
                outUcfId = ucfId;
                return true;
            }
        }
    }
    return false;
}

void LinNode::setEventPending(int ucfId, bool pending) {
    auto it = ucfEventPending.find(ucfId);
    if (it != ucfEventPending.end()) {
        it->second = pending;
    }
}
// -----------------------------------------------------

// -------------------- Inactivity sleep helpers --------------------
void LinNode::resetInactivityTimer() {
    if (inactivitySleepTime < 0) return;

    cancelEvent(inactivityTimer);
    if (state != SLEEP) {
        scheduleAt(simTime() + inactivitySleepTime, inactivityTimer);
    }
}

void LinNode::enterSleep(const char *reason) {
    if (state == SLEEP) return;

    EV_WARN << "Entering Sleep Mode (" << reason << ").\n";
    state = SLEEP;

    cancelEvent(responseTimeoutTimer);
    pendingResponseFrameId = -1;

    resetRxContext();

    if (isMaster) {
        scheduleRunning = false;
        cancelEvent(scheduleTimer);
    }

    cancelEvent(inactivityTimer);
}
// -----------------------------------------------------

void LinNode::handleMessage(cMessage *msg) {

    // --------------------
    // INACTIVITY → SLEEP
    // --------------------
    if (msg == inactivityTimer) {
        enterSleep("inactivity timeout");
        return;
    }

    // --------------------
    // EVENT RE-ARM
    // --------------------
    if (msg == eventRearmTimer) {
        for (auto &kv : ucfEventPending) {
            kv.second = (uniform(0, 1) < eventProbability);
        }
        scheduleAt(simTime() + eventRearmPeriod, eventRearmTimer);
        return;
    }

    // --------------------
    // GO-TO-SLEEP (MASTER LOCAL)
    // --------------------
    if (msg == goToSleepTimer) {
        EV_WARN << "GO-TO-SLEEP (local) completed.\n";
        enterSleep("go-to-sleep completed");
        return;
    }

    // --------------------
    // MASTER SCHEDULE
    // --------------------
    if (msg == scheduleTimer) {
        if (!scheduleRunning) return;

        std::vector<LinSlot> *activeTable =
            resolvingCollision ? &collisionScheduleTable : &scheduleTable;
        int *idx = resolvingCollision ? &collisionSlotIndex : &currentSlotIndex;

        if (activeTable->empty()) return;

        LinSlot &slot = (*activeTable)[*idx];

        pendingHeaderSlot = slot;
        pendingHeaderValid = true;
        endResolveAfterHeader =
            resolvingCollision && (*idx == (int)collisionScheduleTable.size() - 1);

        sendBreakSignal();

        double tBreak = (double)breakBits / baudRate;
        double tDelim = (double)delimiterBits / baudRate;
        scheduleAt(simTime() + tBreak + tDelim + interByteTime, headerTimer);
        scheduleAt(simTime() + slot.period, scheduleTimer);

        *idx = (*idx + 1) % activeTable->size();
        return;
    }

    if (msg == headerTimer) {
        if (!pendingHeaderValid) return;

        LinSlot slot = pendingHeaderSlot;
        pendingHeaderValid = false;

        sendHeaderRemainder(slot.frameId, slot.publisher);

        if (isMaster &&
            !slot.isETF &&
            !isPublisherFor(slot.frameId) &&
            !isDiagnosticFrame(slot.frameId)) {

            pendingResponseFrameId = slot.frameId;

            double timeout =
                1.4 * headerRemainderTxTime() +
                interByteTime +
                1.4 * responseTxTime(dlcForFrame(slot.frameId)) +
                2.0 * propDelay;

            cancelEvent(responseTimeoutTimer);
            scheduleAt(simTime() + timeout, responseTimeoutTimer);
        }


        if (endResolveAfterHeader) {
            resolvingCollision = false;
            collisionSlotIndex = 0;
            endResolveAfterHeader = false;
        }
        return;
    }

    if (msg == responseTimeoutTimer) {
        if (pendingResponseFrameId != -1) {
            errorState.lastTimeoutError = true;
            pendingResponseFrameId = -1;
        }
        return;
    }

    // --------------------
    // WAKEUP LOGIC
    // --------------------
    if (msg == wakeupTimer) {
        if (state == SLEEP || state == IDLE) {
            EV_WARN << "Initiating Wakeup Request (Attempt "
                    << wakeupRetryCount + 1 << ").\n";

            sendWakeupPulse();

            if (state == SLEEP) {
                state = IDLE;
                EV_INFO << "Wakeup initiated locally. Waiting for Master Header confirmation.\n";
            }

            resetRxContext();
            resetInactivityTimer();

            pendingWakeup = true;
            cancelEvent(wakeupExpirationTimer);
            scheduleAt(simTime() + 0.150, wakeupExpirationTimer);
        }
        return;
    }

    if (msg == wakeupExpirationTimer) {
        if (pendingWakeup) {
            if (wakeupRetryCount < 2) {
                EV_WARN << "Wakeup Timeout! No Header from Master. Retrying...\n";
                wakeupRetryCount++;
                scheduleAt(simTime() + 0.01, wakeupTimer);
            } else {
                EV_ERROR << "Wakeup Failed after 3 attempts. Backing off.\n";
                wakeupRetryCount = 0;
                pendingWakeup = false;
            }
        }
        return;
    }

    // -------------------------------------------------
    // CHANGE #2 APPLIES HERE — AND ONLY HERE
    // -------------------------------------------------
    // Any received *bus traffic* counts as activity
    resetInactivityTimer();

    // --------------------
    // SLEEP STATE HANDLING
    // --------------------
    if (state == SLEEP) {
        if (auto brk = dynamic_cast<LinBreakSignal*>(msg)) {
            if (isEchoFromSelf(brk->getSourceAddr())) { delete msg; return; }

            if (isWakeupPulse(brk)) {
                EV_WARN << "WAKEUP PULSE DETECTED! Exiting Sleep Mode.\n";
                state = IDLE;

                resetRxContext();
                resetInactivityTimer();

                if (isMaster) {
                    scheduleRunning = true;
                    if (scheduleTimer->isScheduled()) cancelEvent(scheduleTimer);
                    scheduleAt(simTime() + 0.002, scheduleTimer);
                }
            }
        }
        delete msg;
        return;
    }

    // --------------------
    // BREAK / WAKEUP
    // --------------------
    if (auto brk = dynamic_cast<LinBreakSignal*>(msg)) {
        if (isEchoFromSelf(brk->getSourceAddr())) { delete msg; return; }

        if (isWakeupPulse(brk)) {
            state = IDLE;
            resetRxContext();
        } else {
            state = WAIT_FOR_SYNC;
            rxSawBreak = true;
            rxAwaitingResponse = false;
            rxExpectedResponseId = -1;
        }
        delete msg;
        return;
    }

    // --------------------
    // HEADER
    // --------------------
    if (auto header = dynamic_cast<LinHeader*>(msg)) {
        if (isEchoFromSelf(header->getSourceAddr())) { delete msg; return; }
        // --------------------
        // DIAGNOSTIC HEADER INTERCEPT
        // --------------------
        if (isDiagnosticFrame(header->getFrameId())) {
            handleDiagnosticHeader(header);
            delete msg;
            return;
        }

        if (pendingWakeup) {
            pendingWakeup = false;
            wakeupRetryCount = 0;
            cancelEvent(wakeupExpirationTimer);
        }

        if (!isMaster) {
            if (!rxSawBreak) {
                delete msg;
                return;
            }
            rxSawBreak = false;
        }

        if (header->getPid() != computePID(header->getFrameId())) {
            errorState.lastHeaderError = true;
            delete msg;
            return;
        }

        if (!isMaster) {
            rxAwaitingResponse = true;
            rxExpectedResponseId = header->getFrameId();
        }

        processReceivedHeader(header);
        delete msg;
        return;
    }

    // --------------------
    // RESPONSE
    // --------------------
    if (auto rsp = dynamic_cast<LinResponse*>(msg)) {
        if (isEchoFromSelf(rsp->getSourceAddr())) { delete msg; return; }

        unsigned char expected =
            computeChecksum(rsp, computePID(rsp->getFrameId()));

        if (rsp->getCollision() || rsp->getChecksum() != (char)expected) {
            errorState.lastChecksumError = true;

            if (isMaster && !resolvingCollision) {
                LinSlot &slot =
                    scheduleTable[currentSlotIndex > 0
                        ? currentSlotIndex - 1
                        : scheduleTable.size() - 1];

                if (slot.isETF && !collisionScheduleTable.empty()) {
                    startCollisionResolution();
                }
            }

            delete msg;
            return;
        }

        // --------------------
        // DIAGNOSTIC RESPONSE
        // --------------------
        if (isDiagnosticFrame(rsp->getFrameId())) {
            handleDiagnosticResponse(rsp);
            delete msg;
            return;
        }

        // --------------------
        // NORMAL RESPONSE
        // --------------------
        if (!isMaster) {
            if (!rxAwaitingResponse || rsp->getFrameId() != rxExpectedResponseId) {
                delete msg;
                return;
            }
            rxAwaitingResponse = false;
            rxExpectedResponseId = -1;
        }

        if (isMaster && rsp->getFrameId() == pendingResponseFrameId) {
            cancelEvent(responseTimeoutTimer);
            pendingResponseFrameId = -1;
        }

        if (rsp->getFrameId() == 0x3C &&
            rsp->getDataLength() > 0 &&
            (unsigned char)rsp->getData(0) == 0x00) {
            enterSleep("received go-to-sleep");
        }

        if (state != SLEEP) state = IDLE;
        delete msg;
        return;
    }


    delete msg;
}

bool LinNode::isDiagnosticFrame(int frameId) const {
    return frameId == LIN_DIAG_MASTER_REQUEST_ID ||
           frameId == LIN_DIAG_SLAVE_RESPONSE_ID;
}



// --- HELPERS ---
bool LinNode::isWakeupPulse(LinBreakSignal *brk) {
    double duration = brk->getDuration();
    double bitTime = 1.0 / baudRate;
    return (duration > 20 * bitTime);
}

void LinNode::sendWakeupPulse() {
    LinBreakSignal *brk = new LinBreakSignal("LIN_Wakeup");
    brk->setSourceAddr(nodeAddress);  // true node identity
    brk->setIngressPortIndex(-1);
    brk->setDuration(30.0 / baudRate);
    send(brk, "gate$o");
    resetInactivityTimer();
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
    brk->setSourceAddr(nodeAddress);   // true node identity
    brk->setIngressPortIndex(-1);
    brk->setDuration((double)breakBits / baudRate);
    send(brk, "gate$o");
    resetInactivityTimer();
}

void LinNode::sendHeaderRemainder(int frameId, std::string publisher)
{
    LinHeader *header = new LinHeader("LIN_Header");
    header->setSourceAddr(nodeAddress);
    header->setIngressPortIndex(-1);
    header->setFrameId(frameId);
    header->setPid(computePID(frameId));
    header->setSyncByte(0x55);

    send(header, "gate$o");
    resetInactivityTimer();

    // =================================================
    // FIX 7: MASTER diagnostic request transmission
    // =================================================
    if (isMaster &&
        diagCtx.enabled &&
        frameId == LIN_DIAG_MASTER_REQUEST_ID)
    {
        EV_INFO << "[DIAG] Master sending diagnostic REQUEST payload\n";

        LinResponse *rsp = new LinResponse("LIN_DiagMasterRequest");
        rsp->setSourceAddr(nodeAddress);
        rsp->setIngressPortIndex(-1);
        rsp->setFrameId(LIN_DIAG_MASTER_REQUEST_ID);

        // ---- Single Frame diagnostic request ----
        rsp->setDataLength(3);
        rsp->setData(0, 0x02);                 // SF, length = 2
        rsp->setData(1, LIN_DIAG_NAD_BROADCAST); // NAD
        rsp->setData(2, 0x10);                 // SID: Diagnostic Session Control

        rsp->setChecksum(
            computeChecksum(rsp, computePID(LIN_DIAG_MASTER_REQUEST_ID))
        );

        sendDelayed(rsp, interByteTime, "gate$o");
        state = TRANSMITTING;
        return;
    }

    // =================================================
    // NORMAL (non-diagnostic) publisher behavior
    // =================================================
    if (!isEventFrameId(frameId) && isPublisherFor(frameId)) {
        double delay = headerRemainderTxTime() + interByteTime;
        sendResponse(frameId, delay);
    }
}


void LinNode::processReceivedHeader(LinHeader *header) {
    state = WAIT_FOR_PID;
    int id = header->getFrameId();

    if (isEventFrameId(id)) {
        int ucfId = -1;
        if (selectUcfForEventFrame(id, ucfId)) {
            EV_INFO << "ETF header ID 0x" << std::hex << id << std::dec
                    << ": responding with UCF ID 0x" << std::hex << ucfId << std::dec << endl;
            sendEventResponse(id, ucfId, interByteTime);
            return;
        }
        state = RECEIVING;
        return;
    }

    if (isPublisherFor(id)) {
        EV_INFO << "I am publisher for ID 0x" << std::hex << id << std::dec << endl;
        sendResponse(id, interByteTime);
    } else {
        state = RECEIVING;
    }
}

void LinNode::handleDiagnosticHeader(LinHeader *header)
{
    int frameId = header->getFrameId();

    EV_INFO << "[DIAG] Diagnostic header received. ID=0x"
            << std::hex << frameId << std::dec << endl;

    // BREAK qualification (slaves only)
    if (!isMaster) {
        if (!rxSawBreak) {
            EV_WARN << "[DIAG] Ignoring diagnostic header without BREAK\n";
            return;
        }
        rxSawBreak = false;
    }

    // Header does NOT change TX/RX state
    diagCtx.rxBuffer.clear();
    diagCtx.tpState = DIAG_TP_RX_IN_PROGRESS;

    // Master Request Header (0x3C)
    if (frameId == LIN_DIAG_MASTER_REQUEST_ID) {
        if (!isMaster) {
            EV_INFO << "[DIAG] Slave awaiting diagnostic request payload\n";
        }
        return;
    }

    // Slave Response Header (0x3D)
    if (frameId == LIN_DIAG_SLAVE_RESPONSE_ID) {
        if (isMaster) {
            EV_INFO << "[DIAG] Master awaiting diagnostic response payload\n";
        }
        return;
    }

    EV_WARN << "[DIAG] Unknown diagnostic frame ID\n";
    diagCtx.tpState = DIAG_TP_IDLE;
}


void LinNode::handleDiagnosticResponse(LinResponse *rsp)
{
    EV_INFO << "[DIAG] Diagnostic response received (ID=0x"
            << std::hex << rsp->getFrameId() << std::dec << ")\n";

    // ----------------------------------------
    // Only slaves react to diagnostic requests
    // ----------------------------------------
    if (isMaster) {
        EV_INFO << "[DIAG] Master received diagnostic response payload\n";
        return;
    }


    // ----------------------------------------
    // Only MASTER REQUEST (0x3C) triggers slave TX
    // ----------------------------------------
    if (rsp->getFrameId() != LIN_DIAG_MASTER_REQUEST_ID) {
        return;
    }

    int dlc = rsp->getDataLength();
    if (dlc < 3) {
        EV_WARN << "[DIAG] Invalid diagnostic request length\n";
        diagCtx.tpState = DIAG_TP_IDLE;
        state = IDLE;
        return;
    }

    uint8_t pci = (uint8_t)rsp->getData(0);
    uint8_t frameType = (pci >> 4) & 0x0F;

    // ----------------------------------------
    // Only SINGLE FRAME supported for now
    // ----------------------------------------
    if (frameType != 0x0) {
        EV_WARN << "[DIAG] Unsupported diagnostic request frame type="
                << (int)frameType << endl;
        diagCtx.tpState = DIAG_TP_IDLE;
        state = IDLE;
        return;
    }

    uint8_t payloadLen = pci & 0x0F;
    if (payloadLen < 2) {
        EV_WARN << "[DIAG] Diagnostic SF payload too short\n";
        diagCtx.tpState = DIAG_TP_IDLE;
        state = IDLE;
        return;
    }

    uint8_t nad = (uint8_t)rsp->getData(1);
    uint8_t sid = (uint8_t)rsp->getData(2);

    EV_INFO << "[DIAG] Diagnostic REQUEST received: NAD=0x"
            << std::hex << (int)nad
            << " SID=0x" << (int)sid << std::dec << endl;

    // ----------------------------------------
    // SPEC-ACCURATE NAD + SID HANDLING
    // ----------------------------------------

    bool addressedToMe = (nad == (uint8_t)nodeAddress);
    bool isBroadcast   = (nad == LIN_DIAG_NAD_BROADCAST);

    // Only specific SIDs allow broadcast responses (none implemented yet)
    bool broadcastResponseAllowed = false;

    // ISO 17987: SID 0x10 (Diagnostic Session Control) MUST NOT
    // generate slave responses for broadcast NAD.
    if (!addressedToMe) {
        if (!(isBroadcast && broadcastResponseAllowed)) {
            EV_INFO << "[DIAG] No response permitted for this NAD/SID\n";
            diagCtx.tpState = DIAG_TP_IDLE;
            state = IDLE;
            return;
        }
    }


    // ----------------------------------------
    // Build SLAVE RESPONSE (0x3D)
    // ----------------------------------------
    diagCtx.tpState = DIAG_TP_TX_IN_PROGRESS;
    LinResponse *resp = new LinResponse("LIN_DiagSlaveResponse");
    resp->setSourceAddr(nodeAddress);
    resp->setIngressPortIndex(-1);
    resp->setFrameId(LIN_DIAG_SLAVE_RESPONSE_ID);

    // Single Frame response: PCI + NAD + SID(+0x40)
    resp->setDataLength(3);
    resp->setData(0, 0x02);          // SF, length=2
    resp->setData(1, nad);           // Echo NAD
    resp->setData(2, sid + 0x40);    // Positive response SID

    resp->setChecksum(
        computeChecksum(resp, computePID(LIN_DIAG_SLAVE_RESPONSE_ID))
    );

    EV_INFO << "[DIAG] Slave sending diagnostic RESPONSE (0x3D)\n";

    sendDelayed(resp, interByteTime, "gate$o");
    state = TRANSMITTING;

    // TX is now scheduled; TP transaction complete for SF
    diagCtx.tpState = DIAG_TP_IDLE;
}



void LinNode::applyErrorBits(int logicalFrameId, LinResponse *rsp, int payloadBase) {
    bool hasError = errorState.lastChecksumError || errorState.lastHeaderError || errorState.lastTimeoutError;
    if (!hasError) return;

    for (auto &bind : errorBindings) {
        if (bind.frameId == logicalFrameId) {
            int byteIndex = bind.byteOffset + payloadBase;
            if (byteIndex < 0 || byteIndex >= rsp->getDataLength()) continue;

            char currentByte = rsp->getData(byteIndex);
            currentByte |= (1 << bind.bitIndex);
            rsp->setData(byteIndex, currentByte);
        }
    }

    errorState.lastChecksumError = false;
    errorState.lastHeaderError = false;
    errorState.lastTimeoutError = false;
}

void LinNode::sendResponse(int frameId, double initialDelay) {
    LinResponse *rsp = new LinResponse("LIN_Response");
    rsp->setSourceAddr(nodeAddress);
    rsp->setIngressPortIndex(-1);
    rsp->setFrameId(frameId);

    int dlc = dlcForFrame(frameId);
    rsp->setDataLength(dlc);

    for (int i = 0; i < dlc; i++) rsp->setData(i, (char)0xFF);

    for (auto &sig : signalTable) {
        if (sig.frameId == frameId && sig.byteOffset >= 0 && sig.byteOffset < dlc) {
            rsp->setData(sig.byteOffset, (char)sig.value);
        }
    }

    applyErrorBits(frameId, rsp, /*payloadBase=*/0);

    int pid = computePID(frameId);
    rsp->setChecksum(computeChecksum(rsp, pid));

    sendDelayed(rsp, initialDelay, "gate$o");
    state = TRANSMITTING;

    setEventPending(frameId, false);

    if (isMaster && isGoToSleepCommand(rsp)) {
        cancelEvent(goToSleepTimer);
        scheduleAt(simTime() + initialDelay + responseTxTime(rsp->getDataLength()), goToSleepTimer);
    }

    resetInactivityTimer();
}

void LinNode::sendEventResponse(int etfId, int ucfId, double initialDelay) {
    LinResponse *rsp = new LinResponse("LIN_EventResponse");
    rsp->setSourceAddr(nodeAddress);
    rsp->setIngressPortIndex(-1);
    rsp->setFrameId(etfId);

    int dlc = dlcForFrame(etfId);
    if (dlc < 1) dlc = 1; // must have room for inner PID
    rsp->setDataLength(dlc);

    for (int i = 0; i < dlc; i++) rsp->setData(i, (char)0xFF);

    // First byte carries PID of the UCF
    rsp->setData(0, (char)computePID(ucfId));

    // Remaining bytes carry UCF payload (shifted by +1)
    for (auto &sig : signalTable) {
        if (sig.frameId == ucfId) {
            int target = sig.byteOffset + 1;
            if (target >= 1 && target < dlc) {
                rsp->setData(target, (char)sig.value);
            }
        }
    }

    applyErrorBits(ucfId, rsp, /*payloadBase=*/1);

    int pid = computePID(etfId);
    rsp->setChecksum(computeChecksum(rsp, pid));

    sendDelayed(rsp, initialDelay, "gate$o");
    state = TRANSMITTING;

    setEventPending(ucfId, false);
    resetInactivityTimer();
}

void LinNode::finish() {
    cancelAndDelete(scheduleTimer);
    cancelAndDelete(headerTimer);
    cancelAndDelete(responseTimeoutTimer);
    cancelAndDelete(wakeupTimer);
    cancelAndDelete(wakeupExpirationTimer);
    cancelAndDelete(goToSleepTimer);
    cancelAndDelete(eventRearmTimer);
    cancelAndDelete(inactivityTimer);
}
