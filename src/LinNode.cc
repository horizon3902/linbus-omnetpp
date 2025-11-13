#include "LinNode.h"
#include "LinDataFrame_m.h"
#include "LinUtil.h"

Define_Module(LinNode);

void LinNode::initialize() {
    isMaster = par("isMaster");

    baudRate = par("baudRate");
    propDelay = par("propDelay");
    interByteTime = par("interByteTime");


    sendTimer = new cMessage("sendTimer");

    if (isMaster) {
        EV_INFO << "Master node initialized (own schedule control)\n";

        // Define LIN schedule (Frame IDs and periods)
        scheduleTable.push_back({0x10, "master", 0.01});
        scheduleTable.push_back({0x12, "slave1", 0.01});
        scheduleTable.push_back({0x14, "slave2", 0.01});

        scheduleAt(simTime() + scheduleTable[0].period, sendTimer);
    } else {
        EV_INFO << "Slave node initialized.\n";
    }
}

void LinNode::handleMessage(cMessage *msg) {
    // MASTER node driving the schedule
    if (isMaster && msg == sendTimer) {
        LinSlot &slot = scheduleTable[currentSlotIndex];
        EV_INFO << "Master sending header for Frame ID " << slot.frameId
                << " (publisher: " << slot.publisher << ")" << endl;

        LinDataFrame *header = new LinDataFrame("LIN_Header");
        header->setFrameId(slot.frameId);
        header->setIsResponse(false);

        // Step 3: protocol-correct header fields
        header->setPid(computePID(slot.frameId));
        header->setSyncByte(0x55);
        header->setDataLength(0);   // no data in header


        // realistic header transmission: approximate header bits = break+sync+ID ≈ 34 bits
        int headerBits = 34;
        double headerTx = baudRate > 0 ? (double)headerBits / baudRate : 0.0;

        // schedule send after serialization + propagation
        sendDelayed(header, headerTx + propDelay, "gate$o");


        // Advance to next slot
        currentSlotIndex = (currentSlotIndex + 1) % scheduleTable.size();
        scheduleAt(simTime() + slot.period, sendTimer);
        return;
    }

    // SLAVE or MASTER receiving a message
    LinDataFrame *rx = check_and_cast<LinDataFrame *>(msg);

    // ------------- MASTER receives responses -------------
    if (isMaster && rx->isResponse()) {
        EV_INFO << "Master received response frame ID=" << rx->getFrameId() << endl;
        delete rx;
        return;
    }

    // ------------- SLAVE receives a header ---------------
    if (!isMaster && !rx->isResponse()) {
        if (rx->getFrameId() == 0x12 && strcmp(getName(), "slave1") == 0) {
            EV_INFO << getFullPath() << " responding to header FrameID=0x12\n";
            LinDataFrame *response = new LinDataFrame("LIN_Response");
            response->setFrameId(0x12);
            response->setIsResponse(true);

            response->setPid(computePID(0x12));
            response->setDataLength(8);

            for (int i = 0; i < 8; i++)
                response->setData(i, 'S');

            // Correct checksum
            unsigned char sum = computeChecksum(response);
            response->setChecksum(sum);

            // NOW we log the checksum HERE inside module context
            EV_INFO << "[Checksum] PID=0x" << std::hex << (int)response->getPid()
                    << " length=" << std::dec << response->getDataLength()
                    << " checksum=0x" << std::hex << (int)sum
                    << " (" << (response->getPid() == 0x3C || response->getPid() == 0x3D
                                  ? "classic"
                                  : "enhanced")
                    << ")\n";



            // compute response transmission time: (data bytes + checksum) * 10 bits per byte (start/stop/parity/bit framing)
            int dataBytes = response->getDataLength();
            int responseBits = (dataBytes + 1) * 10; // +1 for checksum byte
            double responseTx = baudRate > 0 ? (double)responseBits / baudRate : 0.0;

            // schedule response serialization + propagation + a small inter-byte slack
            sendDelayed(response, responseTx + propDelay + interByteTime, "gate$o");

        }
        else if (rx->getFrameId() == 0x14 && strcmp(getName(), "slave2") == 0) {
            EV_INFO << getFullPath() << " responding to header FrameID=0x14\n";
            LinDataFrame *response = new LinDataFrame("LIN_Response");
            response->setFrameId(0x14);
            response->setIsResponse(true);

            response->setPid(computePID(0x12));
            response->setDataLength(8);

            for (int i = 0; i < 8; i++)
                response->setData(i, 'T');

            // Correct checksum
            unsigned char sum = computeChecksum(response);
            response->setChecksum(sum);

            // NOW we log the checksum HERE inside module context
            EV_INFO << "[Checksum] PID=0x" << std::hex << (int)response->getPid()
                    << " length=" << std::dec << response->getDataLength()
                    << " checksum=0x" << std::hex << (int)sum
                    << " (" << (response->getPid() == 0x3C || response->getPid() == 0x3D
                                  ? "classic"
                                  : "enhanced")
                    << ")\n";



            // compute response transmission time: (data bytes + checksum) * 10 bits per byte (start/stop/parity/bit framing)
            int dataBytes = response->getDataLength();
            int responseBits = (dataBytes + 1) * 10; // +1 for checksum byte
            double responseTx = baudRate > 0 ? (double)responseBits / baudRate : 0.0;

            // schedule response serialization + propagation + a small inter-byte slack
            sendDelayed(response, responseTx + propDelay + interByteTime, "gate$o");

        }
        else {
            EV_INFO << getFullPath() << " received header ID=" << rx->getFrameId()
                    << " (no action)\n";
        }
    }
    delete rx;
}

void LinNode::finish() {
    cancelAndDelete(sendTimer);
}
