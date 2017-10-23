
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
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
#include <fstream>


#include "lib/md5/md5.cpp"

using namespace std;


bool help = false;
bool reset = false;
string serverDir = "";
string usersFile = "";
int port = 0;
bool isHashed = true;
string user = "";
string pass = "";
int status;


/*
 * Function prints a error message on cerr and exits program
 *
 * @param const char *message message to be printed to cerr
 */
void throwException(const char *message) {
	cerr << message << endl;
	exit(EXIT_FAILURE);
}

bool fileExists(const char *fd) {
	
	struct stat sb;
	
	return (stat(fd, &sb) == 0);
	
}

void printHelp() {
	cout << endl << "Developer: Dominik Skala (xskala11)" << endl;
	cout << "Task name: ISA - POP3 server" << endl;
	cout << "Subject: ISA (2017/2018)" << endl << endl << endl;
	cout
			<< "Math operations solving client can be only used in cooperation with server, that is sending clients their math operations, which all clients solve and send results back."
			<< endl << endl << endl;
	cout << "Usage: popser IP" << endl;
	cout << "Arguments:" << endl;
}

bool checkParams(int argc) {
	if (argc == 1 || (argc > 2 && argc < 7) || argc > 9) {
		throwException("ERROR: Wrong amount of arguments.");
	}
	
	return true;
}

bool parseParams(int argc, char *argv[]) {
	int c;
	while ((c = getopt(argc, argv, ":hcra:p:d:")) != -1) {
		switch (c) {
			case 'h':
				help = true;
				break;
			case 'a':
				usersFile = optarg;
				break;
			case 'c':
				isHashed = false;
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'd':
				serverDir = optarg;
				break;
			case 'r':
				reset = true;
			case ':':
				throwException("ERROR: Wrong arguments.");
				return false;
			case '?':
				throwException("ERROR: Wrong arguments.");
				return false;
			default:
				throwException("ERROR: Wrong arguments.");
				return false;
		}
	}
	return true;
}

string returnSubstring(string String, string delimiter, bool way) {
	
	string subString = "";
	if (String.find(delimiter) != string::npos) {
		if (way) {
			subString = String.substr(String.find(delimiter) + delimiter.length());
		} else {
			subString = String.substr(0, String.find(delimiter));
		}
	}
	
	return subString;
}

string findUserPw(string file, string user) {
	
	string userlist = "";
	string line;
	
	ifstream users (file.c_str());
	if (users.is_open()) {
		while (getline(users, line)) {
			if (line.find(user) != std::string::npos && line.find(user) == 0) {
				return returnSubstring(line, " ", true);
			}
		}
	} else {
		throwException("Error: Couldn't open userpw file.");
	}
	
	users.close();
	
	return "";
}

bool checkUsersFile(const char *name, string &user, string &pass, bool hashed) {
	
	if (!fileExists(name)) {
		return false;
	}
	
	string file = name;
	string line;
	
	ifstream users (file.c_str());
	if (users.is_open()) {
		while (getline(users, line)) {
			if (returnSubstring(line, "username = ", true) != "") {
				user = returnSubstring(line, "=", true);
			} else if (returnSubstring(line, "password = ", true) != "") {
				pass = returnSubstring(line, "password = ", true);
			}
			if (user != "" && pass != "") {
				break;
			}
		}
	}
	
	
	
	return true;
}


int getOperation(string message, bool isHashed) {
	if (message == "NOOP") {
		return 1;
	} else if (message == "QUIT") {
		return 2;
	} else {
		//error op
		return 0;
	}
}

string generatePidTimeStamp(){
	int pid = getpid();
	
	ostringstream pidNum;
	pidNum << pid;
	string pidStr = pidNum.str();
	
	
	char hostname[512];
	hostname[511] = '\0';
	gethostname(hostname,512);
	struct hostent *host;
	host = gethostbyname(hostname);
	
	time_t currTime = time(NULL);
	
	return ("<"+pidStr+"."+to_string(currTime)+"@"+host->h_name);
	
}

void sendResponse(int socket, bool error, string message) {
	

	string response;
	if (error) {
		response = "+OK";
	} else {
		response = "-ERR";
		
	}
	
	response = response+" "+message+"\r\n";
	
	send(socket, response.c_str(), response.size(), 0);
}

void *clientThread(void *threadParam, socket) {
	
	int received;
	auto *tParam = (threadParams *) threadParam;
	
	sendResponse(tParam->socket, , 5);
	for (;;) {
		if ((received = recv(threadParam->commSocket, receivedMessage, 1024, 0)) <= 0) {
			break;
		} else {
			int op = 0;
			op = getOperation(receivedMessage, isHashed);
			
			if (isHashed) {
			
			} else {
			
			}
			
			
			// wait for client
		}
}
	
	int main(int argc, char *argv[]) {
		
		
		// params
		if (!checkParams(argc) || !parseParams(argc, argv)) {
			throwException("ERROR: Wrong arguments.");
		}
		
		if (help) {
			printHelp();
			exit(EXIT_SUCCESS);
		}
		
		
		cout << "HELP VAL: " << help << endl;
		cout << "RESET VAL: " << reset << endl;
		cout << "serverDir VAL: " << serverDir << endl;
		cout << "usersFile VAL: " << usersFile << endl;
		cout << "port VAL: " << port << endl;
		cout << "isHashed VAL: " << isHashed << endl;
		
		// username and pw from config file
		if (!checkUsersFile(usersFile.c_str(), user, pass, isHashed)) {
			throwException("ERROR: Wrong format of User file.");
		}
		
		cout << generatePidTimeStamp() << endl;
		
		cout << serverDir << endl;
		
		// we will create and open a socket
		int serverSocket;
		char receivedMessage[1024];
		
		if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
			throwException("ERROR: Could not open server socket.\n");
		}
		
		
		int opt = 1;
		if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
			throwException("ERROR: Setsockopt failure.");
		}
		
		struct sockaddr_in clientaddr;
		struct sockaddr_in serveraddr;
		memset((char *) &serveraddr, '\0', sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons((unsigned short) port);
		serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
		
		int rc;
		if ((rc = bind(serverSocket, (struct sockaddr *) &serveraddr, sizeof(serveraddr))) < 0) {
			throwException("ERROR: Could not bind to port.\n");
		}
		
		
		if ((listen(serverSocket, 1)) < 0) {
			throwException("ERROR: Could not start listening on port.\n");
		}
		printf("DEBUG INFO\n");
		cout << "Port: " << port << endl;;
		cout << "Server DIR: " << serverDir << endl;
		
		printf("Server is online! It will be now waiting for request.\n");
		
		socklen_t clientlen = sizeof(clientaddr);
		while (1) {
			int commSocket;
			int received = 0;
			
			if ((commSocket = accept(serverSocket, (struct sockaddr *) &clientaddr, &clientlen)) > 0) {
				
				pthread_t thread;
				
				threadParams *thParam = new threadParams;
					thParam->aPath = usersFile;
					thParam->cParam = isHashed;
					thParam->dirPath = serverDir;
					thParam->socket = commSocket;
					thParam->tmpPath = "~/tmpPath/";
					thParam->pidTimeStamp = generatePidTimeStamp();
				
				
				pthread_create(&thread, NULL, clientThread, thParam);
				
			} else {
				break;
			}
		}
	}