
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <string>
#include <mutex>
#include <dirent.h>

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

mutex mutexerino;

typedef struct {
	string mailDir = "";
	string usersFile = "";
	bool isHashed = true;
	string clientUser = "";
	string serverUser = "\0";
	string clientPass = "";
	string serverPass = "\0";
	int commSocket = -1;
	string pidTimeStamp = "";
} threadStruct;


typedef struct mailStruct{
	int id;
	string name;
	int size;
	bool toDelete;
	mailStruct *nextMail;
} *mailStructPtr;

typedef struct {
	mailStructPtr First;
	mailStructPtr Active;
} tList;

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

	// TODO napoveda
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

	// TODO osetreni argumentu

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
	} else if (op == "QUIT") {
		return 4;
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



int numberOfFiles(string dir) {

	int sum = 0;
	DIR *directory;
	struct dirent *ent;

	if ((directory = opendir(dir.c_str())) != NULL) {

		while ((ent = readdir(directory)) != NULL) {
			if(string(ent->d_name) == ".." || string(ent->d_name) == ".")
				continue;
			sum++;
		}

		closedir(directory);

	} else {
		logConsole(true, false, "Error: Number of files from "+dir+" could not be resolved.", true);
	}
	return sum;

}

bool moveNewToCur(threadStruct *tS) {

	DIR *dir;
	struct dirent *ent;

	if ((dir = opendir(tS->mailDir.c_str())) != NULL) {
		while ((ent = readdir(dir)) != NULL) {

		}
	}

	return true;
}


/* List operations over mails */

void initList(tList *L) {
	L->First = NULL;
	L->Active = NULL;
}

void disposeList(tList *L) {
	while (L->First != NULL) {
		mailStructPtr helpMail = new mailStruct;

		helpMail = L->First->nextMail;
		delete(L->First);
		L->First = NULL;
		L->First = helpMail;
		delete(helpMail);
	}

	L->Active = NULL;
}

void insertFirst(tList *L, string name, int size) {

	mailStructPtr first = new mailStruct;

	first->name = name;
	first->size = size;
	first->nextMail = L->First;
	first->toDelete = false;
	L->First->nextMail = first;
}


void first(tList *L) {
	L->Active = L->First;
}


void succ(tList *L) {
	if (L->Active != NULL) {
		L->Active = L->Active->nextMail;
	}
}


void copy(tList *L, int index, int *size, string *name, bool *toDelete) {
	if (L->First == NULL) {
	} else {
		int i = 0;
		while (i < index) {
			first(L);
			if (L->Active != NULL) {
				succ(L);
			}
			i++;
		}
		*size = L->First->size;
		*name = L->First->name;
		*toDelete = L->First->toDelete;
	}
}

/*
void deleteFirst(tList *L) {
	if (L->First != NULL) {
		if (L->First == L->Active) {
			delete(L->Active);
			L->Active = NULL;
		} else {
			delete(L->First);
			L->First = NULL;
		}
	}
}*/

void insertAtTheEnd(tList *L, int size, string name) {
	while (L->Active != NULL) {
		succ(L);
	}

	mailStructPtr last = new mailStruct;

	L->Active->nextMail = last;
	last->size = size;
	last->name = name;
	last->toDelete = false;
}


void markForDeletion(tList *L, int index) {
	int i = 0;
	while (i < index) {
		if (L->Active != NULL) {
			succ(L);
		}
		i++;
	}
	L->Active->toDelete = true;
}

bool checkIndexOfMail(tList *L, int index) {
	int i = 0;
	while (i < index) {

		if (L->Active != NULL) {
			succ(L);
		} else {
			return false;
		}
		i++;
	}
	return true;
}

/* Fill mails in structures from dirs */



void closeConnection(int socket) {
	sendResponse(socket, false, "bye");
	close(socket);
	mutexerino.unlock();
	pthread_exit(NULL);
}

void executeMailServer(threadStruct *tS) {

	if (mutexerino.try_lock()) {
		sendResponse(tS->commSocket, false, "maildrop locked and ready");
		int numOfFiles = numberOfFiles(tS->mailDir+"/cur")+numberOfFiles(tS->mailDir+"/new");
		string stringNumOfFiles = to_string(numOfFiles);
		sendResponse(tS->commSocket, false, "username's maildrop has "+stringNumOfFiles+" messages (xyz octets)");
	} else {
		sendResponse(tS->commSocket, true, "permission denied");
	}


	/* TODO operace na mailech, ziskani obsahu mailu, nahrani do pole a do tmp souboru
	 * velikost v oktetech = velikost v bytech!!!!
	 *
	 *
	 **/




	return;
}


bool userIsAuthorised(int op, threadStruct *tS, char *receivedMessage, bool isHashed) {
	if (!isHashed) {
		if (op == 1) {
			if (checkUser(tS->clientUser = returnSubstring(returnSubstring(receivedMessage, " ", true), " ", false), tS->serverUser)) {
				if (!authenticateUser(tS->clientPass = returnSubstring(returnSubstring(returnSubstring(receivedMessage, " ", true), " ",true), "\r\n", false), tS->serverPass = md5(tS->pidTimeStamp+tS->serverPass))) {
					sendResponse(tS->commSocket, true, "invalid username or password");
					return false;
				}
			} else {
				sendResponse(tS->commSocket, true, "invalid username or password");
				return false;
			}
		} else if (op == 2) {
			if (checkUser(tS->clientUser = returnSubstring(returnSubstring(receivedMessage, " ", true), "\r\n", false), tS->serverUser)) {
				sendResponse(tS->commSocket, false, "now enter password");
				if (((int) recv(tS->commSocket, receivedMessage, 1024, 0)) <= 0) {
					return false;
				} else {
					if ((op = getOperation(receivedMessage)) == 3) {

						if (!authenticateUser(tS->clientPass = returnSubstring(returnSubstring(receivedMessage, " ", true), "\r\n", false), tS->serverPass)) {
							sendResponse(tS->commSocket, true, "invalid username or password");
							return false;
						}
					} else if (op == 4) {
						closeConnection(tS->commSocket);
						return false;
					} else {
						sendResponse(tS->commSocket, true, "invalid operation");
						return false;
					}
				}
			} else {
				sendResponse(tS->commSocket, true, "invalid username or password");
				return false;
			}
		} else if (op == 4) {
			closeConnection(tS->commSocket);
			return false;
		} else {
			sendResponse(tS->commSocket, true, "invalid operation");
			return false;
		}
	} else {
		if (op == 1) {
			if (checkUser(tS->clientUser = returnSubstring(returnSubstring(receivedMessage, " ", true), " ", false),
						  tS->serverUser)) {
				if (!authenticateUser(tS->clientPass = returnSubstring(returnSubstring(returnSubstring(receivedMessage, " ", true), " ", true), "\r\n", false),tS->serverPass = md5(tS->pidTimeStamp + tS->serverPass))) {
					sendResponse(tS->commSocket, true, "invalid username or password");
					return false;
				}
			} else {
				sendResponse(tS->commSocket, true, "invalid username or password");
				return false;
			}
		} else if (op == 4) {
			closeConnection(tS->commSocket);
			return false;
		} else {
			sendResponse(tS->commSocket, true, "invalid operation");
			return false;
		}

	}
	return true;
}


void *clientThread(void *tS) {

	char receivedMessage[1024];
	auto *tParam = (threadStruct *) tS;

	sendResponse(tParam->commSocket, false, "POP3 server ready "+tParam->pidTimeStamp);
	for (;;) {
		if (((int) recv(tParam->commSocket, receivedMessage, 1024, 0)) <= 0) {
			break;
		} else {

			int op = 0;
			if ((op = getOperation(receivedMessage)) != 0) {
				cout <<  endl << "OP: " << op << endl;
				if (userIsAuthorised(op, tParam, receivedMessage, tParam->isHashed)) {
					executeMailServer(tParam);
				} else if (op == 4) {
					closeConnection(tParam->commSocket);
				} else {
					continue;
				}
			} else {
				sendResponse(tParam->commSocket, true, "invalid operation");
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
	string mailDir = "";
	string usersFile = "";
	string serverUser = "";
	string serverPass = "";
	int port = 0;
	bool isHashed = true;

	// params
	if (!checkParams(argc) || !parseParams(argc, argv, help, isHashed, reset, usersFile, mailDir, port)) {
		throwException("ERROR: Wrong arguments.");
	}

	if (help) {
		printHelp();
		exit(EXIT_SUCCESS);
	}


	cout << "HELP VAL: " << help << endl;
	cout << "RESET VAL: " << reset << endl;
	cout << "mailDir VAL: " << mailDir << endl;
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

	cout << mailDir << endl;

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
	cout << "Server DIR: " << mailDir << endl;

	printf("Server is online! It will be now waiting for request.\n");

	socklen_t clientlen = sizeof(clientaddr);
	while (1) {
		int commSocket;

		if ((commSocket = accept(serverSocket, (struct sockaddr *) &clientaddr, &clientlen)) > 0) {

			pthread_t thread;

			tS->serverUser = serverUser;
			tS->serverPass = serverPass;
			tS->isHashed = isHashed;
			tS->mailDir= mailDir;
			tS->commSocket = commSocket;
			tS->pidTimeStamp = generatePidTimeStamp();


			pthread_create(&thread, NULL, clientThread, tS);

		} else {
			break;
		}
	}

	return 0;
}
