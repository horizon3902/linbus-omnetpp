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
    RECEIVING
};

struct LinSlot {
    int frameId;
    std::string publisher;
    double period;
};

// New: Defines a signal (static value for a specific byte in a frame)
struct LinSignal {
    int frameId;
    int byteOffset;
    int value;
};

class LinNode : public cSimpleModule
{
private:
    bool isMaster;
    LinNodeState state;

    cMessage *scheduleTimer;
    cMessage *headerTimer;

    double baudRate;
    double propDelay;
    double interByteTime;

    std::vector<LinSlot> scheduleTable;
    int currentSlotIndex = 0;

    // New: Table of signals this node publishes
    std::vector<LinSignal> signalTable;

    // Helpers
    void parseSchedule(const char* config);
    void parseSignals(const char* config); // <-- New
    bool isPublisherFor(int frameId);      // <-- New

    void sendBreakSignal();
    void sendHeaderRemainder(int frameId, std::string publisher);

    // Updated: Removed 'dataPattern' arg, now uses signalTable
    void sendResponse(int frameId, double initialDelay);

    void processReceivedHeader(LinHeader *header);

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

#endif
