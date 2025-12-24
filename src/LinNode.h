#ifndef LINNODE_H_
#define LINNODE_H_

#include <omnetpp.h>
#include <vector>
#include <string>
#include "LinDataFrame_m.h"

using namespace omnetpp;

enum LinNodeState {
    IDLE,
    WAIT_FOR_SYNC,
    WAIT_FOR_PID,
    TRANSMITTING,
    RECEIVING,
    SLEEP // NEW (Phase 3): Sleep state for power management
};

struct LinSlot {
    int frameId;
    std::string publisher;
    double period;
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
    LinNodeState state;

    // --- TIMERS ---
    cMessage *scheduleTimer;
    cMessage *headerTimer;
    cMessage *responseTimeoutTimer;

    // NEW (Phase 3): Timer to trigger a Wakeup Pulse (simulating external event)
    cMessage *wakeupTimer;

    int pendingResponseFrameId = -1;

    // NEW (Phase 3): Flag to pause/resume master schedule during sleep
    bool scheduleRunning = true;

    // --- PHY PARAMETERS ---
    double baudRate;
    double propDelay;
    double interByteTime;

    // --- SCHEDULE TABLES ---
    std::vector<LinSlot> scheduleTable;
    int currentSlotIndex = 0;

    // Collision Resolution Schedule (Phase 2)
    std::vector<LinSlot> collisionScheduleTable;
    bool resolvingCollision = false;
    int collisionSlotIndex = 0;

    // --- SIGNALS & ERROR MAPPING ---
    std::vector<LinSignal> signalTable;
    std::vector<LinErrorSignalBinding> errorBindings;
    LinErrorState errorState;

    // --- HELPERS ---
    void parseSchedule(const char* config, std::vector<LinSlot>& table);
    void parseSignals(const char* config);
    void parseErrorSignals(const char* config);

    bool isPublisherFor(int frameId);
    void applyErrorBits(int frameId, LinResponse *rsp);

    // --- PROTOCOL ACTIONS ---
    void sendBreakSignal();

    // NEW (Phase 3): Send a long dominant pulse to wake up the bus
    void sendWakeupPulse();

    void sendHeaderRemainder(int frameId, std::string publisher);
    void sendResponse(int frameId, double initialDelay);
    void processReceivedHeader(LinHeader *header);

    // Trigger switch to collision resolution table (Phase 2)
    void startCollisionResolution();

    // NEW (Phase 3): Check if a received break is long enough to be a Wakeup
    bool isWakeupPulse(LinBreakSignal *brk);

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

#endif
