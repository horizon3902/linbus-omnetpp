package src;

network LinNetwork
{
    parameters:
        int numSlaves = default(2); // Can be changed in omnetpp.ini

    submodules:
        bus: LinBusLogic {
            parameters:
                // Bus needs ports for Master (1) + Slaves (numSlaves)
                numGates = 1 + parent.numSlaves;
                baudRate = 19200bps;
                propDelay = 0.0001s;
                interByteTime = 0.0001s;
                @display("p=100,100;t=LINbus");
        }

        master: LinNode {
            parameters:
                isMaster = true;
                baudRate = 19200bps;
                propDelay = 0.0001s;
                interByteTime = 0.0001s;
                @display("p=100,200;t=Master");
        }

        slaves[numSlaves]: LinNode {
            parameters:
                isMaster = false;
                baudRate = 19200bps;
                propDelay = 0.0001s;
                interByteTime = 0.0001s;
                // Arranges slaves in a row for visualization
                @display("p=200,200,row,80;t=Slave");
        }

    connections:
        master.gate <--> bus.gate[0];

        // Connect all slaves to subsequent bus ports
        for i=0..numSlaves-1 {
            slaves[i].gate <--> bus.gate[i+1];
        }
}
