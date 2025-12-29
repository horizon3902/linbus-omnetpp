package src;

simple LinNode
{
    parameters:
        bool isMaster = default(false);

        // Unique node address used for echo filtering.
        // Convention used by LinNetwork.ned:
        // master = 0, slaves[i] = i+1
        int nodeAddress = default(-1);

        // --- SCHEDULE CONFIGURATION (MASTER ONLY) ---
        string scheduleConfiguration = default("");
        string collisionScheduleConfiguration = default("");

        // --- MILESTONE 2: EVENT-TRIGGERED FRAME (ETF) CONFIG ---
        // Format (hex IDs, ';' separated):
        //   "20:21,22;30:31,32"
        string eventFrameConfiguration = default("");
        double eventRearmPeriod @unit(s) = default(-1s);
        double eventProbability = default(1.0);

        // --- SIGNAL CONFIGURATION (ALL NODES) ---
        string signalConfiguration = default("");
        string errorSignalConfiguration = default("");

        // --- PHY LAYER PARAMETERS ---
        double baudRate @unit(bps) = default(19200bps);
        double propDelay @unit(s) = default(0.0001s);
        double interByteTime @unit(s) = default(0.0001s);

        // --- WAKEUP TEST PARAMETER ---
        double wakeupTime @unit(s) = default(-1s);

        // --- MILESTONE 3: INACTIVITY-BASED SLEEP ---
        // If >= 0: node enters SLEEP if it sees no bus activity for this duration.
        // Set small (e.g., 0.2s) for demos; LIN commonly uses ~4s inactivity sleep. [web:10]
        double inactivitySleepTime @unit(s) = default(4s);

    gates:
        inout gate;
}
