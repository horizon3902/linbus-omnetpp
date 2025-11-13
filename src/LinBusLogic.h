#ifndef LINBUSLOGIC_H_
#define LINBUSLOGIC_H_

#include <omnetpp.h>
using namespace omnetpp;

class LinBusLogic : public cSimpleModule {
    struct LinSlot {
        int frameId;
        std::string publisher;
        double period;
    };

    std::vector<LinSlot> scheduleTable;

    // === Step 1: Timing Parameters ===
    double baudRate;        // bits per second (e.g., 19200)
    double propDelay;       // propagation delay (seconds)
    double interByteTime;   // delay between bytes (seconds)

    // Utility to compute transmission time of given bit length
    double computeTxTime(int bits) {
        return (double)bits / baudRate;
    }

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

#endif
