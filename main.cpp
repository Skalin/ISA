
#include "./lib/auth.h"

int main(int argc, char *argv[]) {
	
	
	
	// params
	if (!checkParams(argc) || !parseParams(argv)) {
		throwException("ERROR: Wrong arguments.");
	}
	string authFile;
	
	// username and pw from config file
	if (!checkUsersFile(authFile)) {
		throwException("ERROR: Wrong format of User file.");
	}
	
	// create socket and bind port
	
	// wait for client
}