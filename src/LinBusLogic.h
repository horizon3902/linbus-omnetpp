#ifndef LINBUSLOGIC_H_
#define LINBUSLOGIC_H_

#include <omnetpp.h>
using namespace omnetpp;

class LinBusLogic : public cSimpleModule {
    // Unused struct removed for cleanliness, or kept if you plan to use it later.
    // Keeping minimal required members:

    double baudRate;
    double propDelay;
    double interByteTime;

    // NEW: Error parameters
    double frameErrorRate;
    double frameDropRate;

    // NEW: Collision Detection State
    bool isResponseOnBus;
    cMessage *transmissionEndTimer;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

#endif
