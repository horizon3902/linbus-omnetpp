#include "LinBusLogic.h"
#include <omnetpp.h>

Define_Module(LinBusLogic);

void LinBusLogic::initialize() {
    EV_INFO << "LIN bus initialized (Physical Layer Forwarder)" << endl;
    // Parameter reading can remain if needed
}

void LinBusLogic::handleMessage(cMessage *msg) {
    // We cast to cPacket because all our LIN types (Break, Header, Response)
    // inherit from cPacket (via the .msg definition)
    cPacket *frame = check_and_cast<cPacket *>(msg);

    EV_INFO << "Bus forwarding signal: " << frame->getName()
            << " (" << frame->getBitLength() << " bits)" << endl;

    // Broadcast to all connected nodes
    // Note: In a true electrical bus, signals go to everyone including sender.
    // Logic here forwards to all ports.
    int gateCount = gateSize("gate$o");
    for (int i = 0; i < gateCount; i++) {
        // We dup() for everyone except the last one to avoid memory leaks
        // (Standard OMNeT++ idiom)
        send(i == gateCount - 1 ? frame : frame->dup(), "gate$o", i);
    }
}
