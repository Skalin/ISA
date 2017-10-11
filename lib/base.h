//
// Created by Dominik on 10/10/2017.
//

#ifndef ISA_BASE_H
#define ISA_BASE_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <cstring>
#include <string>
#ifdef _WIN32
	#include <Winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
#endif
#include <sstream>



using namespace std;


void throwException(const char *message);
bool checkParams(int argc);
bool parseParams(char *argv[]);

#endif //ISA_BASE_H
