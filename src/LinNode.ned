package src;

simple LinNode
{
    parameters:
        bool isMaster = default(false);
        
        // --- SCHEDULE CONFIGURATION (MASTER ONLY) ---
        // 1. Normal Schedule: "ID:PublisherName:Period; ..."
        string scheduleConfiguration = default(""); 

        // 2. NEW (Phase 2): Collision Resolution Schedule
        // This schedule is executed only when a collision is detected on the bus.
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

    gates:
        inout gate;
}
