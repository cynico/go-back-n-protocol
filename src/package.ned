package projcet;

@license(LGPL);
//
// TODO documentation
//
network Network
{
    parameters:
        int WS = default(3);
        double TO = default(10.0);
        double PT = default(0.5);
        double TD = default(1.0);
        double ED = default(4.0);
        double DD = default(0.1);
        double LP = default(0.3);
        
    submodules:
        coordinator: Coordinator {
            @display("p=180,173");
        }
        node0: Node {
            @display("p=79,73");
        }
        node1: Node {
            @display("p=306,73");
        } 
    connections:
        coordinator.node1 <--> node1.coordinator;
        coordinator.node0 <--> node0.coordinator;
        node0.peer <--> node1.peer;

}

