#ifndef LINNODE_H_
#define LINNODE_H_

#include <omnetpp.h>
#include <vector>
#include <string>
#include "LinDataFrame_m.h"

using namespace omnetpp;

struct LinSlot {
    int frameId;
    std::string publisher;
    double period;
};

class LinNode : public cSimpleModule
{
private:
    bool isMaster;
    cMessage *sendTimer;

    // Timing parameters
    double baudRate;
    double propDelay;
    double interByteTime;

    // Master schedule table
    std::vector<LinSlot> scheduleTable;
    int currentSlotIndex = 0;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

#endif
