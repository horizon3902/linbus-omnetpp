network LinNetwork
{
    submodules:
        bus: LinBus {
            parameters:
                numGates = 3;  // match node count
        }
        master: LinNode {
            parameters:
                isMaster = true;
                @display("p=100,200");
        }
        slave1: LinNode {
            parameters:
                isMaster = false;
                @display("p=200,200");
        }
        slave2: LinNode {
            parameters:
                isMaster = false;
                @display("p=300,200");
        }
    connections:
        master.gate <--> bus.gate[0];
        slave1.gate <--> bus.gate[1];
        slave2.gate <--> bus.gate[2];
}
