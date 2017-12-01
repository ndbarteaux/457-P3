#ifndef P3_H
#define P3_H

#include <algorithm>
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
#include <climits>

using namespace std;

void parentFunction(string in);
void childFunction(int port, bool debug);

static int MANAGER_PORT = 8880;

struct Children {
    int fd;
    int UDPPort;
    int ID;
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

    // takes a specified signal message and waits for all routers to send that message.
    // once all have been received, sends an ACK to each router for that signal
    int WaitForRouters(string signal) {
        stringstream msg;
        fd_set current = sockets;
        int counter = 0;
        while(true) {
            read_fds = current;
            select(fdmax + 1, &read_fds, NULL, NULL, NULL);
            for (int i = 0; i <= fdmax; i++) {
                if (FD_ISSET(i, &read_fds)) {
                    counter++;
                    char buf[255];
                    int msg_size = recv(i, buf, 255, 0);
//                    buf.remove(msg_size);
                    if (buf != signal) {
                        cerr << "Manager received msg'" << buf << "' - was expecting '" << signal << "'" << endl;
                        exit(1);
                    }
                    int routerID = getID(i);
                    msg << signal << " status received from Router " << routerID;
                    string out = msg.str();
                    msg.str("");
                    writeOutput(out);
                    cout << "Received " << buf << " from " << routerID << endl;
                    FD_CLR(i, &current);

                    // all received
                    if (counter==count) {
                        out = "All router " + string(signal) + " status received. Sending ACK to start next process.";
                        writeOutput(out);
                        for (int j = 0; j < count; j++) {
                            int current_fd = getFD(j);
                            int current_id = getID(current_fd);
                            string response = "ACK" + signal;    // ACKREADY / ACKRFREADY
                            Send(current_fd, response);
                            stringstream msg;
                            msg << "Sent " << response << " to router " << current_id;
                            writeOutput(msg.str());
                        }
                        return 0;
                    }
                }
            }
        }
    }

    int getID(int fd) {
        for (int i=0; i<routers.size(); i++) {
            if (routers[i].fd == fd)
                return routers[i].ID;
        }
    }

    int getFD(int id) {
        for (int i=0; i<routers.size(); i++) {
            if (routers[i].ID == id)
                return routers[i].fd;
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

    string findNeighbors(int id) {
        string result;
        for (int i = 0; i < count; i++) {
			if(lines[i] == "-1") {
				break;
			}
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
	
	void readLines() {
		for(int i=0; i<lines.size(); i++) {
			cout << lines[i] << endl;
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
    int fdmax;
    fd_set read_fds, sockets;
    int server_fd;
    int count;    // number of children
    vector<string> lines;
    vector<Children> routers;

};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



class Router {

public:
    Router(int new_port) {
        router_count = 0;
        pid = getpid();
        port = new_port;
        stringstream out;
        out << "Router Log file " << pid << ".out created";
        string s = out.str();
        writeRouter(s);
    }
    ~Router() {
        if (router_count != 0) {
            for (int i = 0; i < router_count; ++i) {
                delete [] costs[i];
            }
            delete [] costs;
        }
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
            cerr << "getaddrinfo error[][]: " << gai_strerror(status) << endl;
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

        // send udp port to manager
        SendToManager(to_string(port));

        // get response from manager
        string response = RecvFromManager();
        InitializeNeighbors(response);
        cout << router_id << " initialized successfully" << endl;
        if (router_id == 0) {
            for (int i = 0; i < router_count; ++i) {
                cout << ports[i] << endl;
            }
        }
    }


    int SendToManager(string msg) {
//        msg += '\0';
        string out = "Sending the following data to manager.";
        writeRouter(out);
        writeRouter(msg);
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

        string result = "";
        for (int i = 0; i < msg_size - 5; i++) {
            result += static_cast<char>(response[i+4]);
        }
        string out = "Receiving Node information from the manager.";
        writeRouter(out);
        writeRouter(result);
        return result;
    }


    void InitializeCosts() {
        // initialize costs
        costs = new int*[router_count];
        for (int i = 0; i < router_count; ++i) {
            costs[i] = new int[router_count];
        }
        // set all to 0
        for (int i = 0; i < router_count; ++i) {
            for (int j = 0; j < router_count; ++j) {
                costs[i][j] = 0;
            }
        }
        ports = vector<int>(router_count);  // make sure vectors are allocated
        forwarding = vector<int>(router_count);
        total_costs = vector<int>(router_count);
        next_hop = vector<int>(router_count);
    }

    void InitializeNeighbors(string packet) {
        cout << "Made it here" << endl;
        vector<string> tokens = split_string(packet, "|");
        router_id = atoi(tokens[0].c_str());
        router_count = atoi(tokens[1].c_str());
        InitializeCosts();

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
            vector<string> neighbor_data = split_string(tokens[i], " "); // splits up info in 1 neighbor line
            int other_id;
            if (neighbor_data[0] == to_string(router_id)) {
                other_id = atoi(neighbor_data[1].c_str());
            } else {
                other_id = atoi(neighbor_data[0].c_str());
            }
            int cost = atoi(neighbor_data[2].c_str());
            int other_port = atoi(neighbor_data[3].c_str());
            costs[router_id][other_id] = cost;               // store cost in cost grid
            ports[other_id] = other_port; // store port in port table
            stringstream message;
            message << "Neighbor ID - " << other_id << "  Neighbor Cost - " << cost << "  Neighbor Port - " << other_port;
            string output = message.str();
            writeRouter(output);
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
        stringstream out;
        out << "Sent " << msg << " to router with port " << dest_port;
        writeRouter(out.str());
        sendto(udp_fd, msg.c_str(), msg.length(), 0, (struct sockaddr *) &address, sizeof(address));
    }

    string Receive(int fd) {
        struct sockaddr_in remoteaddr;
        socklen_t addrlen = sizeof(remoteaddr);            /* length of addresses */
        char buf[512];
        int numbytes = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *) &remoteaddr, &addrlen);
        buf[numbytes] = '\0';
        int senderPort = ntohs(remoteaddr.sin_port);
        recvPort = senderPort;
        stringstream out;
        out << "Received " << buf << " from neighbor router with port " << senderPort;
        writeRouter(out.str());
        cout << router_id << " received msg: " << buf << endl;
        return string(buf);

    }

    void ReliableFlood() {
        for (int i = 0; i < router_count; i++) {
            if (costs[router_id][i] != 0) {
                // Packet to neighbor |[SenderID]|[SenderID] [Neighbor] [Cost] [NeighborPort]| ...
                cout << router_id << " Neighbor " << i << " Cost: " << costs[router_id][i] << endl;
                string lsp = CreateLSP();
                int sent = Send(ports[i], lsp);
            }
        }
        int counter = 0;
        while (counter < router_count - 1) {
            string packet = Receive(udp_fd);
            if (readLSP(packet)) {
                // forward packet to all neighbors
                for (int i = 0; i < router_count; i++) {
                    if ((costs[router_id][i] != 0) && (ports[i] != recvPort)) {
                        cout << router_id << " forwarding to " << i << endl;
                        int sent = Send(ports[i], packet);
                    }
                }
                counter++;
            }
        }
    }

    struct Hop {
        int id;
        int cost;
        Hop *parent;
    };
    
    bool TreeContains(vector<Hop> tree, int id) {
        for (int i = 0; i < tree.size(); ++i) {
            if (tree[i].id == id) return true;
        }
        return false;
    }
    
    void ShortestPath() {
        int min, new_id, parent_index;
        vector<Hop> tree;
        Hop root = Hop();
        root.id = router_id;
        root.cost = 0;
        root.parent = NULL;
        tree.push_back(root);

        // 1 iteration is one step of the algorithm
        while (tree.size() < router_count) {
            min = INT_MAX;
            new_id = -1;
            // loops through all nodes currently in tree
            for (int i = 0; i < tree.size(); ++i) {
                Hop current_router = tree[i];
                // find least expensive path among current node's neighbors
                for (int j = 0; j < router_count; ++j) {
                    if (costs[current_router.id][j] != 0) { // link exists
                        if (!TreeContains(tree, j)) {   // router already in tree
                            int cost = costs[current_router.id][j] + current_router.cost;
                            if (cost < min) {
                                min = cost;  // update minimum
                                new_id = j;
                                parent_index = i;  // index of parent Hop object in tree vector
                            }
                        }
                    }
                }
            }
            // put new lowest cost router in the tree
            Hop new_hop = Hop();
            new_hop.id = new_id;
            new_hop.cost = min;
            new_hop.parent = &tree[parent_index];
            Hop *temp = new_hop.parent;
            int next;
            if (temp->parent == NULL) {
                next = new_id;
            } else {
                while (temp->parent != NULL && temp->parent->parent != NULL) {
                    temp = temp->parent;
                }
                next = temp->id;
            }
            // temp = neighbor of source = next hop
            next_hop[new_id] = next;
            tree.push_back(new_hop);
        }
        // test print
        cout << router_id << ": ";
        for (int k = 0; k < tree.size(); ++k) {
            cout << tree[k].id << " ";
        }
        cout << endl;
        cout << router_id << ": ";
        for (int k = 0; k < tree.size(); ++k) {
            cout << tree[k].cost << " ";
        }
        cout << endl << endl << "Forwarding table " << router_id << endl;
		string out = "Following is the Shortest Path Forwarding Table.";
		stringstream msg;
		writeRouter(out);
        for (int i = 0; i < router_count; i++) {
            cout << i << ": " << next_hop[i] << endl;
			if(router_id == i) {
				msg << "( " << i << " , " << "0" << " , " << "-" << " )";
			} else {
				msg << "( " << i << " , " << costs[router_id][next_hop[i]] << " , " << next_hop[i] << " )";
			}
			writeRouter(msg.str());
			msg.str("");
        }
    }

    vector<int> next_hop;

    string CreateLSP() {
        stringstream lsp;
        lsp << "|" << router_id << "|";
        for (int i = 0; i < router_count; ++i) {
            if (costs[router_id][i] != 0) {
                lsp << router_id << " " << i << " " << costs[router_id][i] << " " << ports[i] << "|";
            }
        }
        return lsp.str();
    }

    bool readLSP(string lsp) {
        vector<string> tokens = split_string(lsp, "|");
        int recvd_id = atoi(tokens[0].c_str());
        if (HaveReceived(recvd_id)) {
            return false;
        } else {
            for (int i = 1; i < tokens.size(); ++i) {
                vector<string> neighbor_data = split_string(tokens[i], " "); // splits up info in 1 neighbor line
                int other_id;
                if (neighbor_data[0] == to_string(recvd_id)) {
                    other_id = atoi(neighbor_data[1].c_str());
                } else {
                    other_id = atoi(neighbor_data[0].c_str());
                }
                int cost = atoi(neighbor_data[2].c_str());
                int other_port = atoi(neighbor_data[3].c_str());

                costs[recvd_id][other_id] = cost;           // store new cost in cost grid
                if (ports[other_id] == 0) {         // store port IF new
                    ports[other_id] = other_port;
                }
                cout << recvd_id << ": ";
                for (int j = 0; j < router_count; ++j) {  // print for testing
                    cout << costs[recvd_id][j] << " ";
                }
                cout << endl;
            }
            return true;
        }
    }

    bool HaveReceived(int id) {
        bool received = false;
        for (int i = 0; i < router_count; ++i) {
            if (costs[id][i] != 0) {
                return true;
            }
        }
        return false;
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

    void printRouterTable() {
        stringstream msg;
        string out;
        out = "Routing Table:";
        writeRouter(out);
        out = "SourceID		DestID		Cost		DestPort";
        writeRouter(out);
        for(int i=0; i<router_count; i++) {
            for(int j=0; j<router_count; j++) {
                if(costs[i][j] != 0) {
                    msg << "	" << i << " 			" << j << " 			" << costs[i][j] << " 			" << ports[j];
                    out = msg.str();
                    writeRouter(out);
                    msg.str("");
                }
            }
        }
    }

    /** Unpack a 32-bit unsigned from a char buffer  */
    unsigned int Unpack(unsigned char *buf) {
        return (unsigned int) (buf[0]<<24) |
               (buf[1]<<16)  |
               (buf[2]<<8)  |
               buf[3];
    }

    //getters
    int ID() { return router_id; }
    int Count() { return router_count; }

private:
    int **costs;
    int pid;
    int router_id;
    int router_count;
    int port;
    int recvPort;
    int udp_fd;
    int tcp_fd;
    vector<int> forwarding;
    vector<int> ports;
    vector<int> total_costs;

};

#endif
