#ifndef P3_H
#define P3_H

#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>
#include <fcntl.h>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <tuple>
#include <arpa/inet.h>

using namespace std;

void parentFunction(string in);
void childFunction(int fd, int port);


class Manager {

  public:

    Manager(string filename) {
        count = 0;
        ReadFile(filename);
        int output = open("test.txt", O_CREAT|O_TRUNC|O_WRONLY, 0666);
    }

    void ReadFile(string filename) {
        ifstream input(filename);
        string line;
        if (input.is_open()) {
            // children holds how many forks to spawn, read from first line of file
            getline(input, line);
            istringstream numChildren(line);
            numChildren >> this->count;

            while (getline(input, line)) {
                stringstream sstream;
                sstream << line;
                string s = sstream.str();
                lines.push_back(s);
            }
        }
        lines.pop_back();
    }

    int SpawnRouters() {
        // As many pid's as children
        pid_t pid[count];

        // Spawn Children
        int port;
        for(int i=0; i < count; i++) {
            port = (rand() % 999) + 9000;    // sets a random port between 9000 and 9999 for each
            if ((pid[i] = fork()) < 0) {
                perror("Fork failed");
            } else if(pid[i] == 0) {
                childFunction(output, port);  // have child do something (write to file for now)
                exit(0); // so children don't continue to fork
            }
        }
    }

    // Wait for all children to exit
    void Wait() {
        int status;
        // For each specific child
        pid_t pidChild;
        while ((pidChild = wait(&status)) > 0) {
            cout << "Child with PID " << pidChild << " exited with status of " << status << endl;
            count--;
        }
    }

  private:
    int output;   // output file fd
    int count;    // number of children
    vector<string> lines;


};



class Router {

  public:

    Router(int new_port) {
        pid = getpid();
        port = new_port;

        //cout << "Forked child with PID of " << pid << endl;
        //write(output, "Hello World!\n", 13);
    }

    void CreateTCPSocket();

    void CreateUDPSocket() {
        int status;

        udp_fd = socket(AF_INET, SOCK_DGRAM, 0);  // UDP socket fd
        if (udp_fd < 0) {
            perror("Failed to create socket");
            exit(1);
        }

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
    }

  private:
    int pid;
    int port;
    int udp_fd;
    int tcp_fd;

};


#endif
