package src;

simple LinNode
{
    parameters:
        bool isMaster = default(false);
        
        // --- SCHEDULE CONFIGURATION (MASTER ONLY) ---
        // 1. Normal Schedule: "ID:PublisherName:Period; ..."
        // To mark an Event-Triggered Frame (ETF), use the suffix (E) for the publisher.
        // Example: "10:Master:0.01; 20:Slave1(E):0.02"
        string scheduleConfiguration = default(""); 

        // 2. Collision Resolution Schedule
        // This schedule is executed ONLY when a collision is detected on an ETF.
        // It typically polls the associated unconditional frames one by one.
        string collisionScheduleConfiguration = default("");

        // --- SIGNAL CONFIGURATION (ALL NODES) ---
        // Defines the static data values a node publishes
        string signalConfiguration = default("");

        // Defines where the Response_Error bit is located
        string errorSignalConfiguration = default("");

        // --- PHY LAYER PARAMETERS ---
        double baudRate @unit(bps) = default(19200bps);
        double propDelay @unit(s) = default(0.0001s);
        double interByteTime @unit(s) = default(0.0001s);

        // --- WAKEUP TEST PARAMETER ---
        // Time at which this node initiates a wakeup pulse. 
        // Set to -1s to disable.
        double wakeupTime @unit(s) = default(-1s);

    gates:
        inout gate;
}
