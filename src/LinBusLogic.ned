simple LinBusLogic
{
    parameters:
        double baudRate @unit(bps) = default(19200bps);
        double propDelay @unit(s) = default(0.0001s);
        double interByteTime @unit(s) = default(0.0001s);

        int numGates;   // MUST BE PROVIDED BY NETWORK
    gates:
        inout gate[numGates];
}
