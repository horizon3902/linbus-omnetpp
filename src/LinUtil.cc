#include "LinUtil.h"

unsigned char computePID(int frameId)
{
    unsigned char id = frameId & 0x3F;

    unsigned char p0 = ((id >> 0) ^ (id >> 1) ^ (id >> 2) ^ (id >> 4)) & 0x01;
    unsigned char p1 = (~((id >> 1) ^ (id >> 3) ^ (id >> 4) ^ (id >> 5))) & 0x01;

    return id | (p0 << 6) | (p1 << 7);
}

unsigned char computeChecksum(LinDataFrame *frame)
{
    int len = frame->getDataLength();
    unsigned int sum = 0;

    for (int i = 0; i < len; i++)
        sum += (unsigned char)frame->getData(i);

    return (~sum) & 0xFF;
}
