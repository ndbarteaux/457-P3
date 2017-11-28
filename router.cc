#include <cstdio>
#include <cstring>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <tuple>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <fcntl.h>
#include <sstream>

#include "project3.h"

using namespace std;


void childFunction(int output, int port) {
    int pid = getpid();
    //cout << "Forked child with PID of " << pid << endl;
    write(output, "Hello World!\n", 13);


    // UDP socket setup

    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);  // UDP socket fd
    if (udp_fd < 0) {
        perror("Failed to create socket");
        exit(1);
    }

    int status;
    struct sockaddr_in myaddr;      // address object for my address
    memset((char *) &myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);   // fill in local IP
    myaddr.sin_port = htons(port);

    status = bind(udp_fd, (struct sockaddr *) &myaddr, sizeof(myaddr));
    if (status != 0) {
        perror("Socket bind failed");
        exit(1);
    }

    cout << "PID " << pid << "  Binded UDP socket to port " << port << endl;


    // now, we give our port to the manager and get the table of our neighbors (id, link cost, port)

    // send and receive to different ports with sendto() and recvfrom()

}
