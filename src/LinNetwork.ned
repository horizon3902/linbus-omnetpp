network LinNetwork
{
    submodules:
        bus: LinBusLogic {
		    parameters:
		        numGates = 3;
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
                @display("p=100,200;t=MasterECU");
        }

        slave1: LinNode {
            parameters:
                isMaster = false;
                baudRate = 19200bps;
                propDelay = 0.0001s;
                interByteTime = 0.0001s;
                @display("p=200,200;t=Slave1ECU");
        }

        slave2: LinNode {
            parameters:
                isMaster = false;
                baudRate = 19200bps;
                propDelay = 0.0001s;
                interByteTime = 0.0001s;
                @display("p=300,200;t=Slave2ECU");
        }

    connections:
        master.gate <--> bus.gate[0];
        slave1.gate <--> bus.gate[1];
        slave2.gate <--> bus.gate[2];
}
