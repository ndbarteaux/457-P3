#include <stdio.h>
#include <string.h>
#include <iostream> 
#include <sys/types.h> 
#include <sys/wait.h> 
#include <unistd.h>
#include <fstream>
#include <fcntl.h>
#include <sstream>

#include "project3.h"

using namespace std;


void parentFunction(int fd) {
	/*
	int pid = getppid();
	char shmString[12];
	sprintf(shmString, "Manager: Hello World!);
	write(fd, shmString, sizeof(shmString));
	*/
}


int main(int argc, char * argv[]) {
	int fd, children;
	fd = open("test", O_CREAT|O_TRUNC|O_WRONLY, 0666);
	
	string line;
	ifstream numChildren ("input");
	if(numChildren.is_open()) {
		while(getline(numChildren, line)) {
			istringstream ss(line);
			// Children holds how many forks to spawn, read from file.
			ss >> children;					
		}
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
			exit(0);
		}
	}
	
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
