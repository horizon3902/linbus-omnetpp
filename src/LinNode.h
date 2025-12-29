#ifndef LINNODE_H_
#define LINNODE_H_

#include <omnetpp.h>
#include <vector>
#include <string>
#include <unordered_map>
#include "LinDataFrame_m.h"

using namespace omnetpp;

enum LinNodeState {
    IDLE,
    WAIT_FOR_SYNC,
    WAIT_FOR_PID,
    TRANSMITTING,
    RECEIVING,
    SLEEP
};

struct LinSlot {
    int frameId;
    std::string publisher;
    double period;
    bool isETF = false;
};

struct LinSignal {
    int frameId;
    int byteOffset;
    int value;
};

struct LinErrorSignalBinding {
    int frameId;
    int byteOffset;
    int bitIndex;
};

struct LinErrorState {
    bool lastHeaderError = false;
    bool lastChecksumError = false;
    bool lastTimeoutError = false;
};

class LinNode : public cSimpleModule
{
private:
    bool isMaster;
    int nodeAddress;
    LinNodeState state;

    // --- TIMERS ---
    cMessage *scheduleTimer;
    cMessage *headerTimer;
    cMessage *responseTimeoutTimer;

    cMessage *wakeupTimer;
    cMessage *wakeupExpirationTimer;

    // Milestone 1: master must be able to enter sleep even if it ignores echo
    cMessage *goToSleepTimer;

    // Milestone 2: event generation / re-arm timer
    cMessage *eventRearmTimer;

    // Milestone 3: inactivity sleep timer
    cMessage *inactivityTimer;
    double inactivitySleepTime = 4.0;

    int pendingResponseFrameId = -1;
    bool scheduleRunning = true;

    // --- RX QUALIFICATION (FIX) ---
    // For non-master nodes: accept a response only if a valid header was received,
    // and (optionally) only if a BREAK was seen before that header.
    bool rxSawBreak = false;
    bool rxAwaitingResponse = false;
    int  rxExpectedResponseId = -1;

    // --- WAKEUP LOGIC STATE ---
    int wakeupRetryCount = 0;
    bool pendingWakeup = false;

    // --- PHY PARAMETERS ---
    double baudRate;
    double propDelay;
    double interByteTime;

    // --- SCHEDULE TABLES ---
    std::vector<LinSlot> scheduleTable;
    int currentSlotIndex = 0;

    std::vector<LinSlot> collisionScheduleTable;
    bool resolvingCollision = false;
    int collisionSlotIndex = 0;

    // Latch slot used by headerTimer
    LinSlot pendingHeaderSlot;
    bool pendingHeaderValid = false;
    bool endResolveAfterHeader = false;

    // --- SIGNALS & ERROR MAPPING ---
    std::vector<LinSignal> signalTable;
    std::vector<LinErrorSignalBinding> errorBindings;
    LinErrorState errorState;

    // --- MILESTONE 2: ETF MAP + EVENT FLAGS ---
    std::unordered_map<int, std::vector<int>> etfToUcfs;
    std::unordered_map<int, bool> ucfEventPending;

    double eventRearmPeriod = -1;
    double eventProbability = 1.0;

    // --- HELPERS ---
    void parseSchedule(const char* config, std::vector<LinSlot>& table);
    void parseSignals(const char* config);
    void parseErrorSignals(const char* config);

    // Milestone 2
    void parseEventFrames(const char* config);
    void initializeEventFlags();
    bool isEventFrameId(int etfId) const;
    bool selectUcfForEventFrame(int etfId, int& outUcfId) const;
    void setEventPending(int ucfId, bool pending);

    // Milestone 3
    void resetInactivityTimer();
    void enterSleep(const char *reason);

    // FIX
    void resetRxContext();

    bool isPublisherFor(int frameId);
    void applyErrorBits(int logicalFrameId, LinResponse *rsp, int payloadBase);

    bool isGoToSleepCommand(const LinResponse *rsp) const;

    // Timing helpers
    double headerRemainderTxTime() const;
    double responseTxTime(int dataLength) const;

    // --- PROTOCOL ACTIONS ---
    void sendBreakSignal();
    void sendWakeupPulse();
    void sendHeaderRemainder(int frameId, std::string publisher);

    void sendResponse(int frameId, double initialDelay);
    void sendEventResponse(int etfId, int ucfId, double initialDelay);

    void processReceivedHeader(LinHeader *header);

    void startCollisionResolution();
    bool isWakeupPulse(LinBreakSignal *brk);

    bool isEchoFromSelf(int sourceAddr) const {
        return (nodeAddress >= 0 && sourceAddr == nodeAddress);
    }

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

#endif
