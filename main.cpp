
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


typedef struct {
	string serverDir = "";
	string usersFile = "";
	bool isHashed = true;
	string clientUser = "";
	string serverUser = "\0";
	string clientPass = "";
	string serverPass = "\0";
	int commSocket = -1;
	string pidTimeStamp = "";
	string tmpDir;
	int serverStatus = 0; // unauthorized, 1 = authorized - transaction status, 2 end
} threadStruct;


string getCurrDate() {

	char multiByteString[100];
	string dateTime;

	time_t t = time(NULL);
	if (strftime(multiByteString, sizeof(multiByteString), "%T", localtime(&t))) {
		dateTime = multiByteString;
	} else {
		cout << ("Error: Date could not be resolved.") << endl;
	}

	return dateTime;
}

void logConsole(bool logging, bool date, string msg, bool std) {
	if (std) {
		if (date) {
			cerr << getCurrDate()+": "+msg << endl;
		} else {
			cerr << msg << endl;
		}
	} else {
		if (logging) {
			if (date) {
				cout << getCurrDate()+": "+msg;
			} else {
				cout << msg;
			}
		}
	}
}

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

bool parseParams(int argc, char *argv[], bool &help, bool &isHashed, bool &reset, string &usersFile, string &serverDir, int &port) {
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
				user = returnSubstring(line, "username = ", true);
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


int getOperation(string message) {

	string op = returnSubstring(message, "\r\n", false);

	if (returnSubstring(op, " ", false) == "APOP") {
		return 1;
	} else if (returnSubstring(op, " ", false) == "USER") {
		return 2;
	} else if (returnSubstring(op, " ", false) == "PASS") {
		return 3;
	} else {
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
	
	return ("<"+pidStr+"."+to_string(currTime)+"@"+host->h_name+">");
	
}

void sendResponse(int socket, bool error, string message) {
	

	string response;
	if (!error) {
		response = "+OK";
	} else {
		response = "-ERR";
		
	}

	response = response+" "+message+"\r\n";
	
	send(socket, response.c_str(), response.size(), 0);
}


bool checkUser(string clientUser, string serverUser) {

	return (clientUser == serverUser);
}


bool authenticateUser(string clientPass, string serverPass) {

	return (clientPass == serverPass);
}

void executeMailServer(bool hash, int op, threadStruct *tS) {


	authenticateUser(tS->clientPass, tS->serverPass);
	return;
}


bool userIsAuthorised(int op, threadStruct *tS, char *receivedMessage, bool isHashed) {
	if (isHashed) {
		if (op == 1) {
			if (checkUser(tS->clientUser = returnSubstring(returnSubstring(receivedMessage, " ", true), "\r\n", false), tS->serverUser)) {
				if (authenticateUser(tS->clientPass = returnSubstring(returnSubstring(returnSubstring(receivedMessage, " ", true), " ",true), "\r\n", true), tS->serverPass = md5(tS->pidTimeStamp+tS->serverPass))) {
					sendResponse(tS->commSocket, false, "user authorised\r\n");
					tS->serverStatus = 1;
				} else {
					sendResponse(tS->commSocket, true, "invalid username or password\r\n");
					return false;
				}
			}
		} else if (op == 2) {
			if (checkUser(tS->clientUser = returnSubstring(returnSubstring(receivedMessage, " ", true), "\r\n", false), tS->serverUser)) {
				sendResponse(tS->commSocket, false, "now enter password\r\n");

				if (((int) recv(tS->commSocket, receivedMessage, 1024, 0)) <= 0) {
					return false;
				} else {
					if (getOperation(receivedMessage) == 3) {

						if (authenticateUser(tS->clientPass = returnSubstring(returnSubstring(receivedMessage, " ", true), "\r\n", false), tS->serverPass)) {
							sendResponse(tS->commSocket, false, "user authorised\r\n");
							tS->serverStatus = 1;
						} else {
							sendResponse(tS->commSocket, true, "invalid username or password\r\n");
							return false;
						}
					} else {
						sendResponse(tS->commSocket, true, "invalid operation\r\n");
						return false;
					}
				}
			} else {
				sendResponse(tS->commSocket, true, "invalid username or password\r\n");
				return false;
			}
		} else {
			sendResponse(tS->commSocket, true, "invalid operation\r\n");
			return false;
		}
	} else {
		if (op == 1) {
			if (checkUser(tS->clientUser = returnSubstring(returnSubstring(receivedMessage, " ", true), "\r\n", false), tS->serverUser)) {
				if (authenticateUser(tS->clientPass = returnSubstring(returnSubstring(returnSubstring(receivedMessage, " ", true), " ",true), "\r\n", true), tS->serverPass = md5(tS->pidTimeStamp+tS->serverPass))) {
					sendResponse(tS->commSocket, false, "user authorised\r\n");
					tS->serverStatus = 1;
				} else {
					sendResponse(tS->commSocket, true, "invalid username or password\r\n");
					return false;
				}
			}
		} else {
			sendResponse(tS->commSocket, true, "invalid operation\r\n");
			return false;
		}

	}
	return true;
}

void *clientThread(void *tS) {

	char receivedMessage[1024];
	auto *tParam = (threadStruct *) tS;

	sendResponse(tParam->commSocket, false, "POP3 server ready "+tParam->pidTimeStamp+"\r\n");
	for (;;) {
		if (((int) recv(tParam->commSocket, receivedMessage, 1024, 0)) <= 0) {
			break;
		} else {

			int op = 0;
			if ((op = getOperation(receivedMessage)) != 0) {
				cout <<  endl << "OP: " << op << endl;
				if (userIsAuthorised(op, tParam, receivedMessage, tParam->isHashed)) {

				} else {
					continue;
				}
			} else {
				sendResponse(tParam->commSocket, true, "unknown operation");
				cout << op << endl;
				logConsole(true, false, string(receivedMessage), false);
			}

			// wait for client
		}
	}
	return NULL;
}

int main(int argc, char *argv[]) {


	bool help = false;
	bool reset = false;
	string serverDir = "";
	string usersFile = "";
	string serverUser = "";
	string serverPass = "";
	int port = 0;
	bool isHashed = true;

	// params
	if (!checkParams(argc) || !parseParams(argc, argv, help, isHashed, reset, usersFile, serverDir, port)) {
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

	threadStruct *tS = new threadStruct;

	if (!checkUsersFile(usersFile.c_str(), serverUser, serverPass, isHashed)) {
		throwException("ERROR: Wrong format of User file.");
	}

	cout << "USER: " << serverUser << endl;
	cout << "PASS: " << serverPass << endl;

	cout << generatePidTimeStamp() << endl;

	cout << serverDir << endl;

	// we will create and open a socket
	int serverSocket;

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

		if ((commSocket = accept(serverSocket, (struct sockaddr *) &clientaddr, &clientlen)) > 0) {

			pthread_t thread;

			tS->usersFile = usersFile;
			tS->serverUser = serverUser;
			tS->serverPass = serverPass;
			tS->isHashed = isHashed;
			tS->serverDir= serverDir;
			tS->commSocket = commSocket;
			tS->tmpDir = "~/tmpPath/";
			tS->pidTimeStamp = generatePidTimeStamp();


			pthread_create(&thread, NULL, clientThread, tS);

		} else {
			break;
		}
	}

	return 0;
}
