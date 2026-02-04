#include "LinDiag.h"
#include "LinDiagDefs.h"


LinDiag::LinDiag() {}

void LinDiag::init(LinDiagRole role) {
    ctx.enabled = true;
    ctx.role = role;
    ctx.tpState = DIAG_TP_IDLE;
    ctx.rxBuffer.clear();
    ctx.txBuffer.clear();
}

void LinDiag::handleHeader(LinHeader *header) {
    if (!ctx.enabled) return;

    ctx.rxBuffer.clear();
    ctx.tpState = DIAG_TP_RX_IN_PROGRESS;

    EV_INFO << "[DIAG] TP RX started (header only)\n";
}

LinResponse* LinDiag::handleResponse(LinResponse *rsp) {
    if (!ctx.enabled) return nullptr;

    ctx.rxBuffer.clear();
    for (int i = 0; i < rsp->getDataLength(); i++) {
        ctx.rxBuffer.push_back((uint8_t)rsp->getData(i));
    }

    EV_INFO << "[DIAG] TP RX payload size=" << ctx.rxBuffer.size() << endl;
    ctx.tpState = DIAG_TP_IDLE;
    return nullptr;
}

LinResponse* LinDiag::buildSlaveResponse(int sourceAddr) {
    LinResponse *rsp = new LinResponse("LIN_DiagSlaveResponse");

    rsp->setSourceAddr(sourceAddr);
    rsp->setIngressPortIndex(-1);
    rsp->setFrameId(LIN_DIAG_SLAVE_RESPONSE_ID);

    // Single-frame diagnostic response (ISO 17987 SF)
    rsp->setDataLength(3);
    rsp->setData(0, 0x02); // SF, length=2
    rsp->setData(1, 0x7F); // example NAD
    rsp->setData(2, 0x00); // dummy service response

    rsp->setChecksum(0); // calculated by caller
    return rsp;
}
