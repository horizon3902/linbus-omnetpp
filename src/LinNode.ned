simple LinNode
{
    parameters:
        bool isMaster = default(false);
        double sendInterval @unit(s) = default(0.02s);
    gates:
        inout gate;
}
