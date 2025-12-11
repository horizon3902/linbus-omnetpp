package src;

simple LinNode
{
    parameters:
        bool isMaster = default(false);
        
        // Schedule Config (Master only): "ID:PublisherName:Period; ..."
        string scheduleConfiguration = default(""); 

        // Signal Config (All nodes): "ID:ByteOffset:Value; ID:ByteOffset:Value"
        string signalConfiguration = default("");

        double baudRate @unit(bps) = default(19200bps);
        double propDelay @unit(s) = default(0.0001s);
        double interByteTime @unit(s) = default(0.0001s);

    gates:
        inout gate;
}
