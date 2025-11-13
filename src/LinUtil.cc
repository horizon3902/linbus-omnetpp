#include "LinUtil.h"

unsigned char computePID(int frameId)
{
    unsigned char id = frameId & 0x3F;

    unsigned char p0 = ((id >> 0) ^ (id >> 1) ^ (id >> 2) ^ (id >> 4)) & 1;
    unsigned char p1 = (~((id >> 1) ^ (id >> 3) ^ (id >> 4) ^ (id >> 5))) & 1;

    return id | (p0 << 6) | (p1 << 7);
}

unsigned char computeChecksum(LinDataFrame *frame)
{
    unsigned char pid = frame->getPid();
    int len = frame->getDataLength();
    unsigned int sum = 0;

    bool classic = (pid == 0x3C || pid == 0x3D);

    if (!classic)
        sum += pid;

    for (int i = 0; i < len; ++i)
        sum += (unsigned char)frame->getData(i);

    while (sum > 0xFF)
        sum = (sum & 0xFF) + (sum >> 8);

    return (~sum) & 0xFF;
}
