#include "project3.h"

using namespace std;


void childFunction(int output, int port, bool debug) {

    Router router(port);

    router.CreateUDPSocket();
    router.CreateTCPSocket();

    // now, we give our port to the manager and get the table of our neighbors (id, link cost, port)

    // send and receive to different ports with sendto() and recvfrom()

}
