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
    router.SendToManager("LBReady");
    response = router.RecvFromManager();
    cout << router.ID() << " received: " << response << endl;
	router.printRouterTable();
    router.ShortestPath();
	router.SendToManager("DAREADY");
	response = router.RecvFromManager();
	cout << router.ID() << " received: " << response << endl;

//    router.SendToManager("SPTReadyy");



    // now, we give our port to the manager and get the table of our neighbors (id, link cost, port)

    // send and receive to different ports with sendto() and recvfrom()

}
