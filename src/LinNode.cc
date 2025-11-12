#include "LinNode.h"

Define_Module(LinNode);

void LinNode::initialize()
{
    isMaster = par("isMaster");
    sendInterval = par("sendInterval");
    sendTimer = new cMessage("sendTimer");

    if (isMaster) {
        scheduleAt(simTime() + sendInterval, sendTimer);
    }
}

void LinNode::handleMessage(cMessage *msg)
{
    if (msg == sendTimer) {
        // Master creates and broadcasts a LIN frame
        LinDataFrame *frame = new LinDataFrame("LIN_Frame");
        frame->setFrameId(0x12);
        frame->setIsResponse(false);
        frame->setDataLength(8);
        for (int i = 0; i < 8; i++)
            frame->setData(i, 'A' + i);

        send(frame, "gate$o");
        scheduleAt(simTime() + sendInterval, sendTimer);
    }
    else {
        // Slave receives a frame
        LinDataFrame *rx = check_and_cast<LinDataFrame *>(msg);
        EV << getFullPath() << " received frame ID=" << rx->getFrameId() << endl;
        delete rx;
    }
}

void LinNode::finish()
{
    cancelAndDelete(sendTimer);
}
