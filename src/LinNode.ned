simple LinNode
{
    parameters:
        bool isMaster = default(false);
        double sendInterval @unit(s) = default(0.02s);
        double baudRate @unit(bps) = default(19200bps);   // bits per second
		double propDelay @unit(s) = default(0.0001s);
		double interByteTime @unit(s) = default(0.0001s);

    gates:
        inout gate;
}
