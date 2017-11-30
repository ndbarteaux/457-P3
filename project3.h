#ifndef P3_H
#define P3_H

#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <sys/wait.h>
#include <ctime>
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
void childFunction(int port, bool debug);

static int MANAGER_PORT = 8880;

struct Children {
	int fd;
	int UDPPort;
	int ID;
};

struct Link {
    int dest_id;
    int cost;
    int port;
};

class Manager {

  public:
    Manager(string filename) {
        count = 0;
        ReadFile(filename);
		stringstream out;
		out << "Manager Log file created";
		string s = out.str();
		writeOutput(s);
    }

    void ReadFile(string filename) {
        ifstream input(filename);
        string line;
        if (input.is_open()) {
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
                    childFunction(port, true);  // have child do something (write to file for now)
                } else {
                    childFunction(port, false);  // have child do something (write to file for now)
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
        server_fd = socket(server_info->ai_family, server_info->ai_socktype,
                               server_info->ai_protocol); // create socket
        int buf = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &buf, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        bind(server_fd, server_info->ai_addr, server_info->ai_addrlen);

		// Write To Outfile
	    cout << "Manager binded to port " << port.str() << endl;
		stringstream msg;
		msg << "Manager TCP Server Started.";
		string out = msg.str();
		writeOutput(out);
		msg.str("");
		msg << "Manager IP Address - 127.0.0.1    Port No - " << MANAGER_PORT << "    Total Routers - " << count;
		out = msg.str();
		writeOutput(out);

        return server_fd;
    }


    int fdmax;
    fd_set read_fds, sockets;

    int InitialListen() {
        FD_ZERO(&read_fds);
        int newfd;
		
        // Listen on the server socket
		string out = "Listening to routers for TCP connections.";
		writeOutput(out);

        listen(server_fd, 10);  // 10 is max connectors - might need fixing

        FD_SET(server_fd, &sockets);
        fdmax = server_fd;
		int counter = 0;
        while(true) {
            read_fds = sockets;
            select(fdmax + 1, &read_fds, NULL, NULL, NULL);
            for (int i = 0; i <= fdmax; i++) {
                // there is change in a socket
                if (FD_ISSET(i, &read_fds)) {
                    // new connection on the listener
                    if (i == server_fd) {
                        struct sockaddr_in other_address;
                        socklen_t addr_size;

                        // accept connection from child
                        newfd = accept(server_fd, (struct sockaddr *) &other_address, &addr_size);
                        if (newfd < 0) {
                            cerr << "Accept error: file descriptor not valid" << endl;
                        } else {
                            FD_SET(newfd, &sockets);    // add new accepted socket to the set
                            if (newfd > fdmax) { fdmax = newfd; }
                        }
                        if (counter < count) {
                            InitializeRouter(newfd, counter);
                            counter++;
                        }
                        if (counter == count) {
                            string out = "All routers have connected. Sending node address and routing information to routers.";
                            writeOutput(out);
                            for(int j=0; j<count; j++) {
                                string neighbors = findNeighbors(j);
                                stringstream msg;
                                stringstream outFile;
                                msg << "|" << j << "|" << count << "|" << neighbors;
                                int fd = routers[j].fd;
                                Send(fd, msg.str());
                                outFile << "Sent " << msg.str() << " to router " << j;
                                out = outFile.str();
                                writeOutput(out);
                            }
                            return 0;
                        }
                    }
                }
            }
        }
    }

    // after initial listen, wait for all routers to send a ready signal.
    // then, send an ack back to all so the routing algorithm can begin
    void WaitForRouters() {
        fd_set current = sockets;
        int counter = 0;
        while(true) {
            read_fds = current;
            select(fdmax + 1, &read_fds, NULL, NULL, NULL);
            for (int i = 0; i <= fdmax; i++) {
                if (FD_ISSET(i, &read_fds)) {
                    counter++;
                    char buf[255];
                    recv(i, buf, 255, 0);
					int routerID = getID(i);
                    cout << "Received " << buf << " from " << routerID << endl;
                    FD_CLR(i, &current);
                    if (counter==count) {
                        // do something
                    }
                }
            }
        }
    }

	int getID(int fd) {
		for(int i=0; i<routers.size(); i++) {
			if (routers[i].fd == fd) 
				return routers[i].ID;	
		}
	}

	// Fill out a router struct for a given fd and id
    // Calculates its neighbors
    // Constructs init packet to send back to the router (?)
	void InitializeRouter(int sockfd, int id) {
		char buffer[256]; 
		int numbytes = recv(sockfd, buffer, 255, 0);
		int newPort = atoi(buffer);
		cout << buffer << " Received by Manager " << endl;
		routers.push_back(Children());
		routers[id].fd = sockfd;
		routers[id].UDPPort = newPort;
		routers[id].ID = id;

	}

    // constructs and sends a packet to a specific router
    void Send(int fd, string body) {
        body += '\0';
        size_t msg_len = body.length();

        // calculate payload size for the header+body
        unsigned int packet_size = sizeof (unsigned char) * msg_len + 4; // +4 for size header
        unsigned char packet[packet_size];

        /* sets packet[0-3] to represent the 32 bit payload size header */
        Pack(packet, packet_size - 1);
        // fill rest of packet with msg
        for (size_t i = 0; i < msg_len; i++) {
            packet[i + 4] = body[i];
        }
        send(fd, packet, sizeof(packet), 0);
    }
    /* 
	Packet Structure for now
        - Starts with '|' and each field is separated by '|'
        [full packet size]|[Router ID]|[Router count]|Neighbor line 1|Neighbor line 2|...|
    */


    /** store a 32-bit int into a char buffer */
    void Pack(unsigned char *buf, unsigned int i) {
        buf[3] = i & 0x0FF;
        i >>= 8;
        buf[2] = i & 0x0FF;
        i >>= 8;
        buf[1] = i & 0x0FF;
        i >>= 8;
        buf[0] = i;
    }


   /** Unpack a 32-bit unsigned from a char buffer  */
    unsigned int Unpack(unsigned char *buf) {

        return (unsigned int) (buf[0]<<24) |
               (buf[1]<<16)  |
               (buf[2]<<8)  |
               buf[3];
    }


    string findNeighbors(int id) {
        string result;
        for (int i = 0; i < count; i++) {
            string line = lines[i];
            stringstream ss(line);
            int first, second;
            ss >> first >> second;
            // if a match, get port and save line
            if (first == id) {
                int neighbor_port = routers[second].UDPPort;
                result += line + " " + to_string(neighbor_port) + "|";  // 0 1 50 9571|
            } else if (second == id) {
                int neighbor_port = routers[first].UDPPort;
                result += line + " " + to_string(neighbor_port) + "|";  // 0 1 50 9571|
            }
        }
        return result;
    }
	
	void writeOutput(string msg) {
		ofstream output;
		output.open("Manager.out", ofstream::app);
		stringstream s;
		time_t timeStamp = time(nullptr);
		s << asctime(localtime(&timeStamp));
		string stamp = s.str();
		output << "[" << stamp.substr(0, stamp.length()-1) << "]: " << msg << '\n';
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
    int server_fd;
    int count;    // number of children
    vector<string> lines;
	vector<Children> routers;

};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



class Router {

  public:
    Router(int new_port) {
        pid = getpid();
        port = new_port;
		stringstream out;
		out << "Router Log file " << pid << ".out created";
		string s = out.str();
		writeRouter(s);
    }

    void InitializeTCP() {
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
        tcp_fd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol); // create socket

		// Router Logging
		string out = "TCP and UDP Sockets created.";
		writeRouter(out);
		stringstream message;
		message << "TCP Port No - " << MANAGER_PORT;
		out = message.str();
		writeRouter(out);
		message.str("");
		message << "UDP Port No - " << port;
		out = message.str();
		writeRouter(out);
		message.str("");
		out = "Waiting to connect to the manager...";
		writeRouter(out);

        sleep(1);
        status = connect (tcp_fd, server_info->ai_addr, server_info->ai_addrlen);
        if (status == -1) {
            perror(" Error: Failed to connect to manager");
        }
        cout << port << " connected" << endl;
		stringstream portstream;
		portstream << port;
		string udpPort = portstream.str();

		// Router Logging
		out = "Sending the following data to manager.";
		writeRouter(out);
		writeRouter(udpPort);

        // send udp port to manager
        SendToManager(to_string(port));

        // get response from manager
        string response = RecvFromManager();
		out = "Receiving Node information from the manager.";
		writeRouter(out);
		writeRouter(response);
        InitializeNeighbors(response);
        cout << router_id << " initialized successfully" << endl;
    }


    int SendToManager(string msg) {
        cout << "sending: '" << msg << "'" << endl;
        int status =  send(tcp_fd, msg.c_str(), sizeof(msg.c_str()), 0);
        if (status == -1) {
            perror("shit fucked");
        }
    }


    string RecvFromManager() {
        unsigned char response[4096];
        int msg_size = recv(tcp_fd, response, 4096, 0); // receive first chunk

        if (msg_size == 0) { return ""; }
        if (msg_size < 0) {
            perror("Something fucked up receiving");
        }

        int file_size = this->Unpack(response); // processes first 4 bytes of response to get msg length
        cout << "Received packet stating size=" << file_size << endl;

        string result = "";
        for (int i = 0; i < msg_size - 5; i++) {
            result += static_cast<char>(response[i+4]);
        }
        return result;
    }


    void InitializeNeighbors(string packet) {
        vector<string> tokens = split_string(packet, "|");

        router_id = atoi(tokens[0].c_str());
        router_count = atoi(tokens[1].c_str());
		stringstream msg;
		msg << "Router No - " << router_id;
		string out = msg.str();
		writeRouter(out);
		msg.str("");
		msg << "Total Routers - " << router_count;
		out = msg.str();
		writeRouter(out);
		out = "Following are the immediate neighbours.";
		writeRouter(out);
		
        for (int i = 2; i < tokens.size(); ++i) {
			stringstream message;
            vector<string> neighbor_data = split_string(tokens[i], " "); // splits up info in 1 neighbor line
            Link neighbor = Link();
            if (neighbor_data[0] == to_string(router_id)) {
                neighbor.dest_id = atoi(neighbor_data[1].c_str());
            } else {
                neighbor.dest_id = atoi(neighbor_data[0].c_str());
            }
            neighbor.cost = atoi(neighbor_data[2].c_str());
            neighbor.port = atoi(neighbor_data[3].c_str());
			message << "Neighbor ID - " << neighbor.dest_id << "  Neighbor Cost - " << neighbor.cost << "  Neighbor Port - " << neighbor.port;
			string output = message.str();
			writeRouter(output);
            neighbors.push_back(neighbor);
        }
    }

    vector<string> split_string(string s, string delim) {
        vector<string> tokens;
        while (s.length() > 0) {
            if (s.find(delim) == -1) {
                tokens.push_back(s);
                return tokens;
            }
            string tok = s.substr(0, s.find(delim));
            if (tok.length() > 0) {
                tokens.push_back(tok);
            }
            s.erase(0, s.find(delim) + 1);
        }
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

    void Receive(int fd) {
        struct sockaddr_in remoteaddr;
        socklen_t addrlen = sizeof(remoteaddr);            /* length of addresses */
        char buf[256];
        recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *) &remoteaddr, &addrlen);

        cout << port << " received msg: " << buf << endl;
    }

	void writeRouter(string msg) {
		stringstream name;
		name << pid << ".out";
		string filename = name.str();
		ofstream output;
		output.open(filename, ofstream::app);
		stringstream s;
		time_t timeStamp = time(nullptr);
		s << asctime(localtime(&timeStamp));
		string stamp = s.str();
		output << "[" << stamp.substr(0, stamp.length()-1) << "]: " << msg << '\n';
	}

    /** Unpack a 32-bit unsigned from a char buffer  */
    unsigned int Unpack(unsigned char *buf) {
        return (unsigned int) (buf[0]<<24) |
               (buf[1]<<16)  |
               (buf[2]<<8)  |
               buf[3];
    }

private:
    int pid;
    int router_id;
    int router_count;
    int port;
    int udp_fd;
    int tcp_fd;
    vector<Link> neighbors;

};

#endif
