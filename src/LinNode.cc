#include "LinNode.h"

Define_Module(LinNode);

void LinNode::initialize()
{
    isMaster = par("isMaster");
    sendInterval = par("sendInterval");
    sendTimer = nullptr; // master no longer drives its own timer

    if (isMaster)
        EV_INFO << "Master node initialized (bus-driven schedule)\n";
    else
        EV_INFO << "Slave node initialized.\n";
}

void LinNode::handleMessage(cMessage *msg)
{
    LinDataFrame *rx = check_and_cast<LinDataFrame *>(msg);

    // ---------------- MASTER RECEPTION ----------------
    if (isMaster)
    {
        EV_INFO << "Master received frame ID=" << rx->getFrameId() << endl;
        delete rx;
        return;
    }

    // ---------------- SLAVE BEHAVIOUR ----------------
    if (!isMaster)
    {
        if (!rx->isResponse())  // Header from bus
        {
            if (rx->getFrameId() == 0x12 && strcmp(getName(), "slave1") == 0)
            {
                EV_INFO << getFullPath() << " responding to header FrameID=0x12\n";
                LinDataFrame *response = new LinDataFrame("LIN_Response");
                response->setFrameId(0x12);
                response->setIsResponse(true);
                response->setDataLength(8);
                for (int i = 0; i < 8; i++) response->setData(i, 'S');
                send(response, "gate$o");
            }
            else if (rx->getFrameId() == 0x14 && strcmp(getName(), "slave2") == 0)
            {
                EV_INFO << getFullPath() << " responding to header FrameID=0x14\n";
                LinDataFrame *response = new LinDataFrame("LIN_Response");
                response->setFrameId(0x14);
                response->setIsResponse(true);
                response->setDataLength(8);
                for (int i = 0; i < 8; i++) response->setData(i, 'T');
                send(response, "gate$o");
            }
            else
            {
                EV_INFO << getFullPath() << " received header ID=" << rx->getFrameId()
                        << " (no action)\n";
            }
        }
        else
        {
            EV_INFO << getFullPath() << " received response frame ID="
                    << rx->getFrameId() << endl;
        }
        delete rx;
    }
}


void LinNode::finish()
{
    cancelAndDelete(sendTimer);
}
