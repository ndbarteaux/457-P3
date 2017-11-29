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
#include <netdb.h>

using namespace std;

void parentFunction(string in);
void childFunction(int fd, int port, bool debug);

static int MANAGER_PORT = 8880;

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
        } else {
            cerr << "File not found" << endl;
            exit(1);
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
                if (i % 2 == 0) {
                    childFunction(output, port, true);  // have child do something (write to file for now)
                } else {
                    childFunction(output, port, false);  // have child do something (write to file for now)
                }

                exit(0); // so children don't continue to fork
            }
        }
    }

    int CreateTCPSocket() {
        struct addrinfo info;
        struct addrinfo *server_info;  // will point to the results

        memset(&info, 0, sizeof info);
        info.ai_family = AF_INET;
        info.ai_socktype = SOCK_STREAM;
        info.ai_flags = AI_PASSIVE;
        stringstream port;
        port << MANAGER_PORT;

        int status = getaddrinfo(NULL, port.str().c_str(), &info, &server_info);

        if (status != 0) {
            cerr << "getaddrinfo error: " << gai_strerror(status) << endl;
            exit(1);
        }
        int server_fd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol); // create socket
        bind (server_fd, server_info->ai_addr, server_info->ai_addrlen);
        cout << "Manager binded to port " << port.str() << endl;

        listen(server_fd, 10);  // 10 is max connectors - might need fixing

        struct sockaddr_in other_address;
        socklen_t addr_size;

        int other_fd = accept(server_fd, (struct sockaddr *) &other_address, &addr_size);

        cout << other_address.sin_port << endl;
        cout << "Manager found a connection" << endl;
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

    void CreateTCPSocket() {
        struct addrinfo info;
        struct addrinfo *server_info;  // will point to the results
        memset(&info, 0, sizeof info);
        info.ai_family = AF_INET;
        info.ai_socktype = SOCK_STREAM;
        info.ai_flags = AI_PASSIVE;
        stringstream manager_port;
        manager_port << MANAGER_PORT;

        int status = getaddrinfo(NULL, manager_port.str().c_str(), &info, &server_info);

        if (status != 0) {
            cerr << "getaddrinfo error: " << gai_strerror(status) << endl;
            exit(1);
        }
        int manager_fd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol); // create socket
        status = connect (manager_fd, server_info->ai_addr, server_info->ai_addrlen);
        if (status == -1) {
            cerr << port << " Error: Failed to connect to manager" << endl;
        }
        cout << port << " binded" << endl;

        close(manager_fd);

    }

    int CreateUDPSocket() {
        int status;

        udp_fd = socket(AF_INET, SOCK_DGRAM, 0);  // UDP socket fd
        if (udp_fd < 0) {
            perror("Failed to create socket");
            exit(1);
        }

        struct sockaddr_in myaddr;      // address object for my address
        memset(&myaddr, 0, sizeof(myaddr));
        myaddr.sin_family = AF_INET;
        myaddr.sin_addr.s_addr = htonl(INADDR_ANY);   // fill in local IP
        myaddr.sin_port = htons(port);


        status = bind(udp_fd, (struct sockaddr *) &myaddr, sizeof(myaddr));
        if (status != 0) {
            perror("Socket bind failed");
            exit(1);
        }
        cout << "PID " << pid << "  Binded UDP socket to port " << port << endl;
        return udp_fd;
    }

    int Send(int dest_port, string msg) {
        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_port = htons(dest_port);
        inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);  // store localhost address in structure

        string packet = "Hello world";
        sendto(udp_fd, packet.c_str(), sizeof(packet), 0, (struct sockaddr *) &address, sizeof(address));
    }

    void Receive() {
        struct sockaddr_in remoteaddr;
        socklen_t addrlen = sizeof(remoteaddr);            /* length of addresses */
        char buf[256];
        recvfrom(udp_fd, buf, sizeof(buf), 0, (struct sockaddr *) &remoteaddr, &addrlen);

        cout << port << " received msg: " << buf << endl;
    }

  private:
    int pid;
    int port;
    int udp_fd;
    int tcp_fd;

};

#endif
