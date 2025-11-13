network LinNetwork
{
    submodules:
        bus: LinBusLogic {
		    parameters:
		        numGates = 3;
		        baudRate = 19200bps;
		        propDelay = 0.0001s;
		        interByteTime = 0.0001s;
		        @display("p=100,100");
		}


        master: LinNode {
            parameters:
                isMaster = true;
                baudRate = 19200bps;
                propDelay = 0.0001s;
                interByteTime = 0.0001s;
                @display("p=100,200");
        }

        slave1: LinNode {
            parameters:
                isMaster = false;
                baudRate = 19200bps;
                propDelay = 0.0001s;
                interByteTime = 0.0001s;
                @display("p=200,200");
        }

        slave2: LinNode {
            parameters:
                isMaster = false;
                baudRate = 19200bps;
                propDelay = 0.0001s;
                interByteTime = 0.0001s;
                @display("p=300,200");
        }

    connections:
        master.gate <--> bus.gate[0];
        slave1.gate <--> bus.gate[1];
        slave2.gate <--> bus.gate[2];
}
