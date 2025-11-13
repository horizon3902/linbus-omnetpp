#ifndef LINUTIL_H_
#define LINUTIL_H_

#include "LinDataFrame_m.h"

// Computes PID using LIN parity
unsigned char computePID(int frameId);

// Computes classic checksum (sum over data bytes)
unsigned char computeChecksum(LinDataFrame *frame);

#endif
