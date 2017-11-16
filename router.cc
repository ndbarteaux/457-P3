#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<tuple>
#include <sys/wait.h> 
#include <unistd.h>
#include <fstream>
#include <fcntl.h>
#include <sstream>

#include "project3.h"

using namespace std;

void childFunction(int fd) {
	int pid = getpid();
	cout << "Forked child with PID of " << pid << endl;
	write(fd, "Hello World!\n", 13);

}
