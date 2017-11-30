#include "project3.h"

using namespace std;


void childFunction(int port, bool debug) {

    string response;
    Router router(port);

    router.CreateUDPSocket();
    router.InitializeTCP();
    router.SendToManager("READY");
    response = router.RecvFromManager();
    cout << router.ID() << " received: " << response << endl;
    router.ReliableFlood();


    // now, we give our port to the manager and get the table of our neighbors (id, link cost, port)

    // send and receive to different ports with sendto() and recvfrom()

}
