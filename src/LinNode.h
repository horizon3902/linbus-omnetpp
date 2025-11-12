#ifndef LINNODE_H_
#define LINNODE_H_

#include <omnetpp.h>
#include "LinDataFrame_m.h"

using namespace omnetpp;

class LinNode : public cSimpleModule
{
  private:
    bool isMaster;
    double sendInterval;
    cMessage *sendTimer = nullptr;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

#endif
