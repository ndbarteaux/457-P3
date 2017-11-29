#include "project3.h"


void parentFunction(string in) {
    Manager manager(in);
    manager.SpawnRouters();


    // TCP shit, talking to the routers

    manager.Wait();


}


int main(int argc, char * argv[]) {
    if(argc == 1) {
        fprintf(stderr, "Usage for %s: $./manager <input>\n", argv[0]);
        exit(1);
    }
    srand((unsigned int) time(0));  // seed the random # generator
    stringstream ss;
    ss << argv[1];
    string in = ss.str();
    parentFunction(in);

    return 0;
}