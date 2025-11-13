#include "LinBusLogic.h"
#include "LinDataFrame_m.h"

Define_Module(LinBusLogic);

void LinBusLogic::initialize() {
    EV_INFO << "LIN bus initialized (pure forwarder)" << endl;
    baudRate = par("baudRate");
    propDelay = par("propDelay");
    interByteTime = par("interByteTime");

}

void LinBusLogic::handleMessage(cMessage *msg) {
    LinDataFrame *frame = check_and_cast<LinDataFrame *>(msg);

    EV_INFO << "Bus forwarding frame ID " << frame->getFrameId() << endl;

    // Broadcast to all connected nodes
    for (int i = 0; i < gateSize("gate$o"); i++) {
        send(i == gateSize("gate$o") - 1 ? frame : frame->dup(), "gate$o", i);
    }
}



