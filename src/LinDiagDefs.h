#ifndef LIN_DIAG_DEFS_H_
#define LIN_DIAG_DEFS_H_

#include <cstdint>

// =========================
// LIN DIAGNOSTIC FRAME IDs
// =========================

// RAW IDs (parity added later via PID)
constexpr int LIN_DIAG_MASTER_REQUEST_ID = 0x3C;
constexpr int LIN_DIAG_SLAVE_RESPONSE_ID = 0x3D;

// =========================
// DIAGNOSTIC CONSTANTS
// =========================
constexpr uint8_t LIN_DIAG_NAD_BROADCAST = 0x7F;

#endif
