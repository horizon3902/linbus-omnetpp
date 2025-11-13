#ifndef LINUTIL_H_
#define LINUTIL_H_

#include "LinDataFrame_m.h"

unsigned char computePID(int frameId);

// returns checksum only, no logging here
unsigned char computeChecksum(LinDataFrame *frame);

#endif
