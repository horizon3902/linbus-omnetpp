#include "LinUtil.h"

unsigned char computePID(int frameId)
{
    unsigned char id = frameId & 0x3F;

    // LIN 2.0+ Parity calculation
    // P0 = ID0 ^ ID1 ^ ID2 ^ ID4
    unsigned char p0 = ((id >> 0) ^ (id >> 1) ^ (id >> 2) ^ (id >> 4)) & 1;
    // P1 = ~(ID1 ^ ID3 ^ ID4 ^ ID5)
    unsigned char p1 = (~((id >> 1) ^ (id >> 3) ^ (id >> 4) ^ (id >> 5))) & 1;

    return id | (p0 << 6) | (p1 << 7);
}

unsigned char computeChecksum(LinResponse *frame, int pid)
{
    int len = frame->getDataLength();
    unsigned int sum = 0;

    // Check for Classic Checksum (used for diagnostics 0x3C, 0x3D)
    // or if the node is configured for LIN 1.x (not modeled here, assuming LIN 2.x)
    bool classic = (pid == 0x3C || pid == 0x3D);

    // Enhanced Checksum includes the PID
    if (!classic) {
        sum += pid;
    }

    // Add Data Bytes
    for (int i = 0; i < len; ++i) {
        sum += (unsigned char)frame->getData(i);
        // Handle carry immediately or after loop - LIN standard uses inverted modulo 256 with carry
        // Simple addition loop here, strict carry handling below
    }

    // Fold carries (add high byte to low byte)
    while (sum > 0xFF) {
        sum = (sum & 0xFF) + (sum >> 8);
    }

    // Invert result
    return (~sum) & 0xFF;
}
