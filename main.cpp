
#include <iostream>
#include <ctime>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <string>
#include <mutex>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sstream>
#include <fstream>
#include <signal.h>


#include "lib/md5/md5.cpp"

using namespace std;

mutex mutexerino;


string mailConfig = "./mail.cfg";
string mailDir = "";

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
	string name;
	size_t size;
	bool toDelete;
	mailStruct *nextMail;
} *mailStructPtr;

typedef struct {
	mailStructPtr First;
	mailStructPtr Active;
} tList;


tList *L = new tList;

/*
 * Function prints an error message on cerr and exits program
 *
 * @param const char *message message to be printed to cerr
 */
void throwException(const char *message) {
	cerr << message << endl;
	exit(EXIT_FAILURE);
}

/*
 * Function checks if the folder is regular folder, it is typically called after itemExists(), it shouldn't be called without this function
 *
 * @param const char *file name of the folder
 * @returns true if folder exists and is regular folder, false otherwise
 */
bool fileExists(const char *file) {
	struct stat sb;
	return (stat(file, &sb) == 0);
}


/*
 * Function checks if the folder is regular folder, it is typically called after itemExists(), it shouldn't be called without this function
 *
 * @param const char *folder name of the folder
 * @returns true if folder exists and is regular folder, false otherwise
 */
bool isDirectory(const char *folder) {
	struct stat sb;
	return (fileExists(folder) && S_ISDIR(sb.st_mode));
}


/*
 * Function prints help message on cout and informs user about usage of program
 *
 */
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


/*
 * Function checks if amount of params is ok
 *
 * @param int argc amount of arguments -1
 */
bool checkParams(int argc) {

	// TODO
	/* if ((argc > 1 && argc < 7) || argc > 9) {
		throwException("ERROR: Wrong amount of arguments.");
	}*/
	
	return true;
}


/*
 * Function parses params and stores their value in arguments
 *
 * @param int argc amount of arguments
 * @param char *argv[] array of arguments
 * @param bool &help address of variable where the value of help param will be stored
 * @param bool &isHashed address of variable where the value of which form of authorisation is available will be stored
 * @param bool &reset address of variable where the value whether the reset will be applied will be stored
 * @param string &mailDir address of variable where the value of path to maildir will be stored
 * @param int &port address of variable where the value of port will be stored
 * @returns bool true if params are ok, false otherwise
 */
bool parseParams(int argc, char *argv[], bool &help, bool &isHashed, bool &reset, string &usersFile, string &mailDir, int &port) {
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
				mailDir = optarg;
				break;
			case 'r':
				reset = true;
				break;
			case ':':
				throwException("ERROR: Wrong 1 arguments.");
				return false;
			case '?':
				throwException("ERROR: Wrong 2 arguments.");
				return false;
			default:
				throwException("ERROR: Wrong 3 arguments.");
				return false;
		}
	}
	return true;
}


/*
 * Function returns substring from the given string by delimiter and depending on way of search
 *
 * @param string String which is searched
 * @param string delimiter which is used to split the String
 * @param bool way - true will parse substring to the right, false is to the left
 * @returns string parsed substring, if nothing is found, returns "", else returns the parsed string
 */
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


/*
 * Function checks validity of user file
 *
 * @param const char *name name of the file
 * @param string &user address of variable where the value of username will be stored
 * @param string &pass address of variable where the value of password will be stored
 * @returns true if validation was successful, false otherwise
 */
bool checkUsersFile(const char *name, string &user, string &pass) {

	string line;

	if (!fileExists(name)) {
		return false;
	}
	ifstream users (name);
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
	users.close();
	return true;
}


/*
 * Function checks the received message and parses the correct name of operation. Depending of the type, it returns a value of operation specified in documentation
 *
 * @param string message message that is being parsed
 * @returns int operation integer representation of operation to remember and check it betterS
 */
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
	} else if (op == "LIST") {
		return 5;
	} else if (returnSubstring(op, " ", false) == "LIST") {
		return 6;
	} else if (op == "NOOP") {
		return 7;
	} else if (op == "STAT") {
		return 8;
	} else if (returnSubstring(op, " ", false) == "RETR") {
		return 9;
	} else if (returnSubstring(op, " ", false) == "DELE") {
		return 10;
	} else if (op == "RSET") {
		return 11;
	} else if (op == "UIDL") {
		return 12;
	} else if (returnSubstring(op, " ", false) == "UIDL") {
		return 13;
	} else if (returnSubstring(op, " ", false) == "TOP") {
		return 14;
	} else {
		return 0;
	}
}


/*
 * Function generates a timestamp with hostname and pid in it
 * Time in this function is a amount of seconds that passed since 1.1.1970, pid is a current process id, hostname is hostname of the server
 *
 * @returns string pidTimeStamp in format <PID.time@hostname>
 */
string generatePidTimeStamp(){
	char hostname[512];
	int pid = getpid();
	struct hostent *host;
	time_t currTime = time(nullptr);
	
	ostringstream pidNum;
	pidNum << pid;
	string pidStr = pidNum.str();

	hostname[511] = '\0';
	gethostname(hostname,512);
	host = gethostbyname(hostname);
	return ("<"+pidStr+"."+to_string(currTime)+"@"+host->h_name+">");
}


/*
 * Function sends response to the client with a +OK or -ERR prefix
 *
 * @param int socket defines where to send the message
 * @param bool error defines whether the error has occured or not (true ERROR, false ALRIGHT)
 * @param string message message that will be sent to the client after the prefix
 */
void sendResponse(int socket, bool error, string message) {
	string response;
	error ? response = "+OK" : response = "-ERR";
	response = response+" "+message+"\r\n";
	send(socket, response.c_str(), response.size(), 0);
}


/*
 * Function sends only a message to the client, without additional info
 *
 * @param int socket defines where to send the message
 * @param string message that will be sent to the client
 */
void sendMessage(int socket, string message) {
	message = message+"\r\n";
	send(socket, message.c_str(), message.size(), 0);
}


/*
 * Simple function for checking usernames
 *
 * @param string clientUser user received from client
 * @param string serverUser user on server
 * @returns true if clients are ok, false otherwise
 */
bool checkUser(string clientUser, string serverUser) {
	return (clientUser == serverUser);
}


/*
 * Simple function for checking user password
 *
 * @param string clientPass user received from client
 * @param string serverPass user on server
 * @returns true if client passwords are ok, false otherwise
 */
bool authenticateUser(string clientPass, string serverPass) {
	return (clientPass == serverPass);
}


/*
 * Function copies file from one path to another
 *
 * @param string from where the file is stored
 * @param string to where we want to paste the file
 */
void copyFile(string from, string to) {
	ifstream src(from.c_str(), ios::binary);
	ofstream dst(to.c_str(), ios::binary);

	dst << src.rdbuf();
}


/*
 * Function checks if the mail exists in the folder
 *
 * @param string dir folder of the server
 * @param string name name of the mail
 *
 * @returns true if mail exists, false otherwise
 */
bool mailExists(string dir, string name) {
	DIR *directory;
	struct dirent *ent;

	if ((directory = opendir(dir.c_str())) != nullptr) {
		while ((ent = readdir(directory)) != nullptr) {
			if (string(ent->d_name) == "." || string(ent->d_name) == "..") {
				continue;
			}
			if (ent->d_name == name) {
				return true;
			}
		}
	}

	return false;
}


/*
 * Function deletes the mail from the given path
 *
 * @param string path full path of the email containing also its name
 */
void deleteMail(string path) {
	remove(path.c_str());
}


/*
 * Function moves all mails from maildir/new to maildir/cur, during that it renames the mail so it containes the ":2," flag
 *
 * @param threadStruct *tS thread structure containing mail info
 */
void moveNewToCur(threadStruct *tS) {

	DIR *dir;
	struct dirent *ent;


	string mailDirNew = tS->mailDir+"/new/";
	string mailDirCur = tS->mailDir+"/cur/";

	cout << mailDirNew << endl;
	cout << mailDirCur << endl;

	if ((dir = opendir(mailDirNew.c_str())) != nullptr) {
		while ((ent = readdir(dir)) != nullptr) {
			if (string(ent->d_name) == "." || string(ent->d_name) == "..") {
				continue;
			}
			if (mailExists(mailDirCur, ent->d_name)) {
				cout << "MAIL EXISTS" << endl;
				cout << mailDirCur+ent->d_name << endl;
				deleteMail(mailDirCur+ent->d_name);
			}
			cout << ent->d_name << endl;
			copyFile(mailDirNew+ent->d_name, mailDirCur+ent->d_name+":2,");
			deleteMail(mailDirNew+ent->d_name);
		}
	}

}


/* List operations over mails */

void initList(tList *L) {
	L->First = nullptr;
	L->Active = nullptr;
}

void first(tList *L) {
	L->Active = L->First;
}

void disposeList(tList *L) {

	while (L->First != NULL) {
		mailStructPtr helpMail = new mailStruct;

		helpMail = L->First->nextMail;
		delete(L->First);
		L->First = nullptr;
		L->First = helpMail;
		delete(helpMail);
	}

	L->Active = nullptr;
}

void insertFirst(tList *L, string name, size_t size) {

	mailStructPtr first = new mailStruct;

	first->name = name;
	first->size = size;
	first->nextMail = L->First;
	first->toDelete = false;

	first->nextMail = L->First;
	L->First = first;
}


void succ(tList *L) {
	if (L->Active != nullptr) {
		L->Active = L->Active->nextMail;
	}
}


void copySize(tList *L, int index, size_t *size) {
	if (L->First == nullptr) {
	} else {
		first(L);
		int i = 0;
		while (i < index) {
			if (L->Active->nextMail != nullptr) {
				succ(L);
			}
			i++;
		}
		*size = L->Active->size;
	}
}

void copyName(tList *L, int index, string *name) {
	if (L->First == nullptr) {
	} else {
		int i = 0;
		while (i < index) {
			first(L);
			if (L->Active != nullptr) {
				succ(L);
			}
			i++;
		}
		*name = L->First->name;
	}
}

void copyToDelete(tList *L, int index, bool *toDelete) {
	if (L->First == nullptr) {
	} else {
		int i = 0;
		while (i < index) {
			first(L);
			if (L->Active != nullptr) {
				succ(L);
			}
			i++;
		}
		*toDelete = L->First->toDelete;
	}
}






/*
void deleteFirst(tList *L) {
	if (L->First != nullptr) {
		if (L->First == L->Active) {
			delete(L->Active);
			L->Active = nullptr;
		} else {
			delete(L->First);
			L->First = nullptr;
		}
	}
}*/

void insertAtTheEnd(tList *L, string name, size_t size) {

	first(L);

	while (L->Active->nextMail != nullptr) {
		succ(L);
	}

	mailStructPtr last = new mailStruct;

	L->Active->nextMail = last;
	last->size = size;
	last->name = name;
	last->toDelete = false;
	last->nextMail = nullptr;
}

bool checkIfMarkedForDeletion(tList *L, int index) {
	first(L);

	int i = 0;
	while (i < index) {
		succ(L);
		i++;
	}

	return L->Active->toDelete;
}

void markForDeletion(threadStruct *tS, tList *L, int index) {

	if (checkIfMarkedForDeletion(L, index)) {
		sendResponse(tS->commSocket, true, "mail already marked for deletion");
	} else {
		L->Active->toDelete = true;
		sendResponse(tS->commSocket, false, "mail marked for deletion");
	}
}

bool checkIndexOfMail(tList *L, int index) {
	int i = 0;
	first(L);
	while (i < index) {
		if (L->Active->nextMail != nullptr) {
			succ(L);
		} else {
			return false;
		}
		i++;
	}

	return true;
}

string returnNameOfMail(tList *L, int index) {
	int i = 0;

	first(L);

	while(i < index) {
		i++;
		succ(L);
	}

	return L->Active->name;
}

int sumOfMails(tList *L) {
	int i = 0;

	first(L);
	while (L->Active->nextMail != nullptr) {
		if (!L->Active->toDelete) {
			i++;
		}
		succ(L);
	}

	return i;
}

size_t sumOfSizeMails(tList *L) {
	size_t size = 0;
	first(L);
	while(L->Active != nullptr) {
		if (!L->Active->toDelete) {
			size += L->Active->size;
		}
		succ(L);
	}

	return size;
}


void getMailContent(threadStruct *tS, string name) {
	string returnedString;
	string line;

	ifstream file;
	file.open(tS->mailDir+"/cur/"+name);
	while (getline(file, line)) {
		sendMessage(tS->commSocket, line);
	}

	file.close();
}


/* Fill mails in structures from dirs */



void closeConnection(int socket) {
	sendResponse(socket, false, "bye");
	close(socket);
	mutexerino.unlock();
	pthread_exit(nullptr);
}


bool checkMailDir(string dir) {
	return (isDirectory((dir+"/new").c_str()) && isDirectory((dir+"/cur").c_str()) && isDirectory((dir+"/tmp").c_str()));
}

void rsetOperation(threadStruct *tS) {
	first(L);

	while(L->Active != nullptr) {
		if (L->Active->toDelete) {
			L->Active->toDelete = false;
		}
		if (L->Active->nextMail == nullptr) {
			break;
		}
	}
	sendResponse(tS->commSocket, true, "user's maildrop has "+to_string(sumOfMails(L))+" messages ("+to_string(sumOfSizeMails(L))+") octets");
}

void deleOperation(threadStruct *tS, int index) {
	if (!checkIndexOfMail(L, index)) {
		sendResponse(tS->commSocket, true, "mail does not exist");
	} else {
		markForDeletion(tS, L, index);
	}
}

void statOperation(threadStruct *tS) {
	int mails = sumOfMails(L);
	size_t size = sumOfSizeMails(L);
	sendResponse(tS->commSocket, false, to_string(mails)+" "+to_string(size));
}


/*
 * Function does just nothing, typical NOOP, it is here just so the application looks nice and that every operation has its own function
 *
 * @param threadStruct *tS thread structure containing maildir info and other useful informations
 */
void noopOperation(threadStruct *tS) {
}


/*
 * Function returns the params of mail on certain index
 *
 * @param threadStruct *tS thread structure containing maildir info and other useful informations
 * @param int index index of mail we are looking for
 */
void listIndexOperation(threadStruct *tS, int index) {
	if (!checkIndexOfMail(L, index)) {
		sendResponse(tS->commSocket, true, "mail does not exist");
	} else {
		if (checkIfMarkedForDeletion(L, index)) {
			sendResponse(tS->commSocket, true, "mail marked for deletion");
		} else {
			size_t size;
			copySize(L, index, &size);
			sendResponse(tS->commSocket, false, to_string(size)+" octets");
		}
	}

}


/*
 * Function returns info about the email almost the same as the list, but it also returns the content of mail including its header
 *
 * @param threadStruct *tS thread structure containing maildir info and other useful informations
 * @param int index index of mail we are looking for
 */
void retrOperation(threadStruct *tS, int index) {
	if (!checkIndexOfMail(L, index)) {
		sendResponse(tS->commSocket, true, "mail does not exist");
	} else {
		if (checkIfMarkedForDeletion(L, index)) {
			sendResponse(tS->commSocket, true, "mail marked for deletion");
		} else {
			listIndexOperation(tS, index);
			getMailContent(tS, returnNameOfMail(L, index));
			sendMessage(tS->commSocket, ".");
		}
	}

}


//TODO commentary
void listOperation(threadStruct *tS) {
	sendResponse(tS->commSocket, false, ""+to_string(sumOfMails(L))+" messages ("+to_string(sumOfSizeMails(L))+" octets)");
	// TODO send one by one
}


/*
 * Function returns size of file in octets (size_t)
 *
 * @param string file name of the file
 * @returns size of the file in octets
 */
size_t getFileSize(string file) {
	ifstream fileStream(file.c_str(), fstream::binary);

	fileStream.seekg(0, fileStream.end);
	size_t size = fileStream.tellg();
	fileStream.seekg(0);

	fileStream.close();
	return size;
}


/*
 * Function gets all emails from maildir and fills them into the global list of mails
 *
 * @param threadStruct *tS thread structure containing maildir info and other useful informations
 */
void createListFromMails(threadStruct *tS) {

	initList(L);

	int i = 0;
	size_t size = 0;
	string name = "";
	DIR *dir;
	struct dirent *ent;

	if ((dir = opendir((tS->mailDir+"/cur/").c_str())) != nullptr) {
		while ((ent = readdir(dir)) != nullptr) {
			size = getFileSize(tS->mailDir+"/cur/"+ent->d_name);
			name = ent->d_name;
			if (string(ent->d_name) == ".." || string(ent->d_name) == ".") {
				continue;
			} else {
				if (!i) {
					insertFirst(L, name, size);
				} else {
					insertAtTheEnd(L, name, size);
				}
				i++;
			}
		}
	}


}


/*
 * Function validates a request depending on operation
 *
 * @param string received message that is being validated
 * @param int operation operation that should be processed
 */
bool validateRequest(string received, int operation) {

	return true;
}


/*
 * TODO
 */
void executeMailServer(threadStruct *tS) {

	if (mutexerino.try_lock()) {
		if (checkMailDir(tS->mailDir)) {
			sendResponse(tS->commSocket, false, "maildrop locked and ready");
		} else {
			sendResponse(tS->commSocket, true, "maildir not OK");
			closeConnection(tS->commSocket);
		}
	} else {
		sendResponse(tS->commSocket, true, "permission denied");
	}

	moveNewToCur(tS);

	createListFromMails(tS);

	sendResponse(tS->commSocket, false, "username's maildrop has "+to_string(sumOfMails(L))+" messages ("+to_string(sumOfSizeMails(L))+" octets)");

	for(;;) {
		char received[1024];
		if (recv(tS->commSocket, received, 1024, 0) <= 0) {
			break;
		} else {
			int op;
			if ((op = getOperation(received)) != 0) {
				cout << op << endl;
				if (validateRequest(string(received), op)) {

					// operations
					if (op <= 3) {
						sendResponse(tS->commSocket, true, "already authorised");
					} else if (op == 5) {
						listOperation(tS);
					} else if (op == 6) {
						listIndexOperation(tS, stoi(returnSubstring(returnSubstring(string(received), "\r\n", false), " ", true), nullptr));
					} else if (op == 7) {
						noopOperation(tS);
					} else if (op == 8) {
						statOperation(tS);
					} else if (op == 9) {
						retrOperation(tS, stoi(returnSubstring(returnSubstring(string(received), "\r\n", false), " ", true), nullptr));
					} else if (op == 10) {
						deleOperation(tS, stoi(returnSubstring(returnSubstring(string(received), "\r\n", false), " ", true), nullptr));
					} else if (op == 11) {

					} else if (op == 12) {

					} else if (op == 13) {

					} else if (op == 14) {

					} else {
						sendResponse(tS->commSocket, true, "invalid operation");
					}
				} else {
					sendResponse(tS->commSocket, true, "invalid operation");
				}
			}
		}
	}

}


/*
 * Function authorises user depending on his credentials. It selectes between two modes, hashed and unhashed mode
 * If the param -c was passed to the program, it will accept only nonhashed version of password, otherwise it accepts only hashed version of password
 * Hashing is done on the client side using md5 depending on the pidTimeStamp that the server sends
 *
 * @param int op value of operation to check correct order
 * @param threadStruct *tS thread structure containing maildir info and other useful informations
 * @param char *receivedMessage messsage that is being parsed and from which the authorisation data are taken
 * @param bool isHashed param that selects mode of authorisation (hashed/non-hashed)
 * @returns bool true if user is authorised correctly, false otherwise
 */
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


/*
 * TODO
 */
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
			}

		}
	}
	return nullptr;
}


/*
 * TODO
 */
void resetMail() {

	string line;

	ifstream users (mailConfig.c_str());
	if (users.is_open()) {
		int i = 0;
		string mailDir;
		string name;
		while(getline(users, line)) {
			if (i % 2 == 0) {
				mailDir = returnSubstring(line, "dir = ", true);
			} else {
				name = returnSubstring(line, "name = ", true);
				cout << name << endl;
				copyFile(mailDir+"/cur/"+name, mailDir+"/new/"+returnSubstring(name, ":2,", false));
				remove((mailDir+"/cur/"+name).c_str());
			}
			i++;
		}

	remove(mailConfig.c_str());
	}

}


/*
 * Function creates a mail config, that is saved in the same folder as the server, this function is fundamental for -reset param
 * Format of this config is following:
 * maildirdir = dir
 * name = name:2, (:2, is already in name) name = returnSubstring(name, ":2,", false)
 * **/
void createMailCfg(tList *L) {

	string maildir = mailDir;
	string name;
	ofstream file;
	file.open(mailConfig);

	first(L);

	while (L->Active != nullptr) {
		name = L->Active->name;
		file << "name = " << name << endl;
		file << "dir = " << maildir << endl;
		succ(L);
	}

	file.close();
}

/*
 * Function checks whether sigint was passed, if yes, the server correctly ends its process
 *
 * @param int param not used
 */
void siginthandler(int param) {
	createMailCfg(L);
	disposeList(L);
	exit(1);
}


/*
 * TODO indexing of mails = now starts at 0, should be starting at 1
 *
 */
int main(int argc, char *argv[]) {

	signal(SIGINT, siginthandler);

	bool help = false;
	bool reset = false;
	string usersFile;
	string serverUser;
	string serverPass;
	int port = 0;
	bool isHashed = true;

	// params
	if (!checkParams(argc) || !parseParams(argc, argv, help, isHashed, reset, usersFile, mailDir, port)) {
		throwException("ERROR: Wrong arguments.");
	}

	printf("DEBUG INFO\n");
	cout << "Arguments: " << argc << endl;
	cout << "Port: " << port << endl;
	cout << "HELP VAL: " << help << endl;
	cout << "RESET VAL: " << reset << endl;
	cout << "mailDir VAL: " << mailDir << endl;
	cout << "usersFile VAL: " << usersFile << endl;
	cout << "port VAL: " << port << endl;
	cout << "isHashed VAL: " << isHashed << endl;
	cout << "USER: " << serverUser << endl;
	cout << "PASS: " << serverPass << endl;
	cout << "Mail Config FILE: " << mailConfig << endl;

	// help param was passed? if yes, then print help and end program
	if (help) {
		printHelp();
		exit(EXIT_SUCCESS);
	}

	// reset param wass passed? if yes, reset mail, if reset was the only param, end program
	if (reset) {
		resetMail();
		if (argc == 2) {
			exit(EXIT_SUCCESS);
		}
	}

	// username and pw from config file
	if (!checkUsersFile(usersFile.c_str(), serverUser, serverPass)) {
		throwException("ERROR: Wrong format of User file.");
	}

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
	printf("Server is online! It will be now waiting for request.\n");

	socklen_t clientlen = sizeof(clientaddr);
	while (1) {
		int commSocket;

		if ((commSocket = accept(serverSocket, (struct sockaddr *) &clientaddr, &clientlen)) > 0) {

			pthread_t thread;

			threadStruct *tS = new threadStruct;

			tS->serverUser = serverUser;
			tS->serverPass = serverPass;
			tS->isHashed = isHashed;
			tS->mailDir= mailDir;
			tS->commSocket = commSocket;
			tS->pidTimeStamp = generatePidTimeStamp();

			cout << "TMSTMP: " << tS->pidTimeStamp << endl;


			pthread_create(&thread, NULL, clientThread, tS);

		} else {
			break;
		}
	}

	return 0;
}
