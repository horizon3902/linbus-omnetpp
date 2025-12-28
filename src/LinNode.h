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
    SLEEP
};

struct LinSlot {
    int frameId;
    std::string publisher;
    double period;
    // NEW: ISO Compliance - Track if this slot is for an Event-Triggered Frame
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
    LinNodeState state;

    // --- TIMERS ---
    cMessage *scheduleTimer;
    cMessage *headerTimer;
    cMessage *responseTimeoutTimer;

    // Wakeup timers
    cMessage *wakeupTimer;           // Triggers the initial pulse (simulated external event)
    cMessage *wakeupExpirationTimer; // Tracks the 150ms timeout waiting for Master header

    int pendingResponseFrameId = -1;

    // Flag to pause/resume master schedule during sleep
    bool scheduleRunning = true;

    // --- WAKEUP LOGIC STATE ---
    int wakeupRetryCount = 0; // ISO allows 3 retries
    bool pendingWakeup = false; // True if we sent a pulse and are waiting for header

    // --- PHY PARAMETERS ---
    double baudRate;
    double propDelay;
    double interByteTime;

    // --- SCHEDULE TABLES ---
    std::vector<LinSlot> scheduleTable;
    int currentSlotIndex = 0;

    // Collision Resolution Schedule
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
    void sendWakeupPulse();
    void sendHeaderRemainder(int frameId, std::string publisher);
    void sendResponse(int frameId, double initialDelay);
    void processReceivedHeader(LinHeader *header);

    // Trigger switch to collision resolution table
    void startCollisionResolution();

    // Check if a received break is long enough to be a Wakeup
    bool isWakeupPulse(LinBreakSignal *brk);

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

#endif
