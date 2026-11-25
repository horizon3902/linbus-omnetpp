#ifndef LIN_DIAG_H_
#define LIN_DIAG_H_

#include <vector>
#include <omnetpp.h>
#include "LinDataFrame_m.h"

using namespace omnetpp;

enum LinDiagRole {
    DIAG_NONE,
    DIAG_MASTER,
    DIAG_SLAVE
};

enum LinDiagTpState {
    DIAG_TP_IDLE,
    DIAG_TP_RX_IN_PROGRESS,
    DIAG_TP_TX_IN_PROGRESS
};

struct LinDiagContext {
    bool enabled = false;
    LinDiagRole role = DIAG_NONE;
    LinDiagTpState tpState = DIAG_TP_IDLE;

    uint8_t expectedSeq = 0;
    uint8_t nextTxSeq = 0;

    std::vector<uint8_t> rxBuffer;
    uint16_t expectedLength = 0;

    std::vector<uint8_t> txBuffer;
    uint16_t txOffset = 0;

    uint8_t targetAddress = 0x7F;
    uint8_t sourceAddress = 0x00;

    simtime_t lastActivity;

    // Spec: Slave diagnostic response is deferred until master transmits 0x3D header
    bool pendingSlaveResponse = false;
    LinResponse *pendingResponse = nullptr;
};

#endif
