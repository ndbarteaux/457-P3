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
	string line;
	vector<string> lines;
	fd = open("test.txt", O_CREAT|O_TRUNC|O_WRONLY, 0666);
	ifstream input ("input.txt");

	if(input.is_open()) {

		// children holds how many forks to spawn, read from first line of file
		getline(input, line);
		istringstream numChildren(line);
		numChildren >> children;

		while(getline(input, line)) {
			stringstream sstream;
			sstream << line;
			string s = sstream.str();
			lines.push_back(s);
		}
	}

	lines.pop_back();
	for(auto v : lines) {
		cout << v << "\n" << endl;           
	}                                        
                                             
	// As many pid's as children             
	pid_t pid[children];	
	// Spawn Children
	for(int i=0; i<children; i++) {
		if ((pid[i] = fork()) < 0) {
			perror("Fork failed");
		} else if(pid[i] == 0) {
			childFunction(fd);  // have child do something (write to file for now)
			exit(0); // so childs don't continue to fork
		}
	}
}


int main(int argc, char * argv[]) {
	if(argc == 1) {
		fprintf(stderr, "Usage for %s: $./manager <input.txt>", argv[0]);
		exit(1);
	} 
	parentFunction();
	// Wait for children to exit
	int status;
	// For each specific child
	pid_t pidChild;
	while ((pidChild = wait(&status)) > 0) {
		cout << "Child with PID " << pidChild << " exited with status of " << status << endl;
		children--;
	}
	return 0;
}
