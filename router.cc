#include "project3.h"

using namespace std;


void childFunction(int output, int port) {

    Router router(port);

    router.CreateUDPSocket();

    // UDP socket setup




    // now, we give our port to the manager and get the table of our neighbors (id, link cost, port)

    // send and receive to different ports with sendto() and recvfrom()

}
