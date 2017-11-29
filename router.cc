#include "project3.h"

using namespace std;


void childFunction(int output, int port, bool debug) {

    Router router(port);

    router.CreateUDPSocket();

    // UDP socket setup


    int otherport;
    if (debug) {
        sleep(1);
        cout << port << " Enter port: ";
        cin >> otherport;
        router.Send(otherport, "");
    } else {
        router.Receive();
    }

    // now, we give our port to the manager and get the table of our neighbors (id, link cost, port)

    // send and receive to different ports with sendto() and recvfrom()

}
