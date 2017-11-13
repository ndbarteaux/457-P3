#include <iostream> 
#include <sys/types.h> 
#include <sys/wait.h> 
#include <unistd.h>

#include "project3.h"

using namespace std;

void childFunction() {
	pid_t childID = getpid();
	cout << "Child PID: " << childID << endl;
}
