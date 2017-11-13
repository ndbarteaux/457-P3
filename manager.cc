#include <iostream> 
#include <sys/types.h> 
#include <sys/wait.h> 
#include <unistd.h>

#include "project3.h"

using namespace std;


void parentFunction() {
	pid_t parentID = getppid();
	cout << "Parent ID: " << parentID << endl;
}


int main(int argc, char * argv[]) {
	pid_t pid;
	
	pid = fork();
	if(pid == 0) {
		childFunction();
	} else {
		int status;
		parentFunction();
		wait(&status);
		cout << "Child exited with PID: " << status << endl;
	}
	
	return 0;
}
