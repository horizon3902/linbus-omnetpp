cplusplus {{
#include <omnetpp.h>
}}

packet LinDataFrame {
    int frameId;
    bool isResponse;
    int pid;          // protected identifier
    char syncByte = 0x55;

    int dataLength = 8;
    char data[8];

    char checksum;    // final checksum byte
}
