#include <stdio.h>
#include <string.h>
#include <iostream> 
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include<vector>
#include <sys/wait.h> 
#include <unistd.h>
#include <fstream>
#include <fcntl.h>
#include <sstream>

#include "project3.h"

using namespace std;
int children;

void parentFunction() {
	int fd, children;
	vector<string> vec;
	fd = open("test", O_CREAT|O_TRUNC|O_WRONLY, 0666);
	
	string line;
	int i = 0;
	ifstream numChildren ("input");
	if(numChildren.is_open()) {
		while(getline(numChildren, line)) {
			if(line.compare("-1") == 1)
				break;
			if (i = 0) {
				istringstream ss(line);
				// Children holds how many forks to spawn, read from file. Always first line
				ss >> children;	
			}
			i=1;
			stringstream sstream;
			sstream << line;
			string s = sstream.str();
			vec.push_back(s);
		}
	}
	vec.pop_back();
	for(auto v : vec) {
		cout << v << "\n" << endl;
	}

	// As many pid's as children
	pid_t pid[children];	
	// Spawn Children
	for(int i=0; i<children; i++) {
		if ((pid[i] = fork()) < 0) {
			perror("Fork failed");
		} else if(pid[i] == 0) {
			// Have child do something (write to file for now)
			childFunction(fd);
		}
	} 
}


int main(int argc, char * argv[]) {
	if(argc == 1) {
		fprintf(stderr, "Usage for %s: $./manager <input>", argv[0]);
		exit(1);
	} 
	parentFunction();
	// Wait for children to exit
	int status;
	// For each specific child
	pid_t pidChild;
	while(children > 0) {
		pidChild = wait(&status);
		cout << "Child with PID " << pidChild << " exited with status of " << status << endl;
		children--;
	}
	return 0;
}
