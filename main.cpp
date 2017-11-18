#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <mutex>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
#include <fstream>
#include <csignal>
#include <vector>
#include <list>
#include <algorithm>
#include "md5.h"
#include "main.h"

using namespace std;

mutex mutex1;


string cwd;
string mailConfig = "mail.cfg";

vector<threadStruct> threads;

list <mailStruct> mailList;

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
 * @returns bool true if folder exists and is regular folder, false otherwise
 */
bool fileExists(const char *file) {
	struct stat sb;
	return (stat(file, &sb) == 0);
}


/*
 * Function checks if the folder is regular folder, it is typically called after itemExists(), it shouldn't be called without this function
 *
 * @param const char *folder name of the folder
 * @returns bool true if folder exists and is regular folder, false otherwise
 */
bool isDirectory(const char *folder) {
	struct stat sb;
	return ((stat(folder, &sb) == 0) && S_ISDIR(sb.st_mode));
}


/*
 * Function prints help message on cout and informs user about usage of program
 *
 */
void printHelp() {
	cout << "\r\nDeveloper: Dominik Skala (xskala11)" << endl;
	cout << "Task name: ISA - POP3 server" << endl;
	cout << "Subject: ISA (2017/2018)\r\n\r\n" << endl;
	cout << "Description:" << endl;
	cout << "\tPOP3 Server able to handle single user. Handles standard POP3 operations, like RETR, TOP, etc. More information can be found in documentation.\r\n\r\n" <<  endl;
	cout << "Usage:" << endl;
	cout << "\t./popser [-h] [-a PATH] [-c] [-p PORT] [-d PATH] [-r]" << endl;
	cout << "Arguments:" << endl;
	cout << "\t[-h]\t\tPrints the help and ends the program. Can be used with any other param, nevertheless, if this param is used, the program ends immediately after printing help." << endl;
	cout << "\t[-c]\t\tSelects type of authentification of user. If the param is passed to the program, server accepts raw passwords, if the param is not passed, server accepts only hashed version. More in documentation." << endl;
	cout << "\t[-r]\t\tIf the arguments is given, the server resets and returns all mails to the folders before the start initial start of the mail server." << endl;
	cout << "\t[-a PATH]\tSpecifies the path to the auth file, which contains login information of single user. Must be used with -p and -d arguments. Arguments: -h, -c and -r are optional." << endl;
	cout << "\t[-p PORT]\tSpecifies the port on which the server will run. Must be used with -a and -d arguments. Arguments: -h, -c and -r are optional." << endl;
	cout << "\t[-d PATH]\tSpecifies the path to the Maildir, which must contain /cur, /new and /tmp folders. Also should contain all mails. Must be used with -p and -a arguments. Arguments: -h, -c and -r are optional." << endl;
}


/*
 * Function checks if amount of params is ok
 *
 * @param int argc amount of arguments -1
 * @returns bool true if params were alright, false otherwise
 */
bool checkParams(int argc) {

	// TODO
	/*if ((argc > 1 && argc < 7) || argc > 9) {
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
 * @param bool &isHashed address of variable where the value of which form of authorization is available will be stored
 * @param bool &reset address of variable where the value whether the reset will be applied will be stored
 * @param string &mailDir address of variable where the value of path to mail directory will be stored
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
				if (port < 0 || port > 65535) {
					throwException("ERROR: Wrong range of port");
				}
				break;
			case 'd':
				mailDir = optarg;
				break;
			case 'r':
				reset = true;
				break;
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
/*
	if (reset) {
		if (argc > 3) {
			if (mailDir == "" || usersFile == "" || !port) {
				throwException("ERROR: Wrong arguments");
			}
		}
	} else {
		if (mailDir == "" || usersFile == "" || !port) {
			throwException("ERROR: Wrong arguments");
		}
	}*/
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
 * @returns bool true if validation was successful, false otherwise
 */
bool checkUsersFile(const char *name, string &user, string &pass) {

	string line;

	if (!fileExists(name)) {
		return false;
	}
	ifstream users (name);
	if (users.is_open()) {
		getline(users, line);
			if (returnSubstring(line, "username = ", true) != "") {
				user = returnSubstring(line, "username = ", true);
				getline(users, line);
				if (returnSubstring(line, "password = ", true) != "") {
					pass = returnSubstring(line, "password = ", true);
				}
			}
		if (user == "" || pass == "") {
				return false;
		}
		users.close();
	}
	return true;
}


/*
 * Function checks the received message and parses the correct name of operation. Depending of the type, it returns a value of operation specified in documentation
 *
 * @param string message message that is being parsed
 * @returns int operation integer representation of operation to remember and check it betterS
 */
int getOperation(string message) {

	string command = returnSubstring(message, "\r\n", false);
	transform(command.begin(), command.end(), command.begin(), ::tolower);

	if (returnSubstring(command, " ", false) == "apop") {
		return 1;
	} else if (returnSubstring(command, " ", false) == "user") {
		return 2;
	} else if (returnSubstring(command, " ", false) == "pass") {
		return 3;
	} else if (command == "quit") {
		return 4;
	} else if (command == "list") {
		return 5;
	} else if (returnSubstring(command, " ", false) == "list") {
		return 6;
	} else if (command == "noop") {
		return 7;
	} else if (command == "stat") {
		return 8;
	} else if (returnSubstring(command, " ", false) == "retr") {
		return 9;
	} else if (returnSubstring(command, " ", false) == "dele") {
		return 10;
	} else if (command == "rset") {
		return 11;
	} else if (command == "uidl") {
		return 12;
	} else if (returnSubstring(command, " ", false) == "uidl") {
		return 13;
	} else if (returnSubstring(command, " ", false) == "top") {
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
	string pidTimeStamp = "<"+pidStr+"."+to_string(currTime)+"@"+host->h_name+">";
	int uniq = 0;
	if (!isPidUnique(pidTimeStamp)) {
		pidTimeStamp = generatePidTimeStamp();
	}
	return (pidTimeStamp);
}


/*
 * Function returns the current working directory
 *
 * @returns string cwd current working directory
 */
string getWorkindDirectory() {

	string wd;
	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		wd = string(cwd);
	}
	return wd;
}



/*
 * Function checks the uniqueness of pidTimeStamp, if the pidTimeStamp is not unique, it will generate new one and store it in correct thread
 *
 * @param string PTS pidTimeStamp string
 * @returns true if pidTimeStamp is unique, false otherwise
 */
bool isPidUnique(string PTS) {
	bool status = true;
	int counter = 1;
	for(std::vector<threadStruct>::iterator it = threads.begin(); it != threads.end(); ++it) {
		if (it->pidTimeStamp == PTS) {
			it->pidTimeStamp = generatePidTimeStamp();
			status = false;
		}
	}
	return status;
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
	error ? response = "-ERR" : response = "+OK";
	if (message == "") {
		response = response+"\r\n";
	} else {
		response = response+" "+message+"\r\n";
	}
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
 * @returns bool true if clients are ok, false otherwise
 */
bool checkUser(string clientUser, string serverUser) {
	return (clientUser == serverUser);
}


/*
 * Simple function for checking user password
 *
 * @param string clientPass user received from client
 * @param string serverPass user on server
 * @returns bool true if client passwords are ok, false otherwise
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
 * @returns bool true if mail exists, false otherwise
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
bool deleteFile(string path) {
	return remove(path.c_str());
}


/*
 * Function moves all mails from maildir/new to maildir/cur, during that it renames the mail so it contains the ":2," flag
 *
 * @param threadStruct *tS thread structure containing mail info
 */
void moveNewToCur(threadStruct *tS, string name) {


	string mailDirNew = tS->mailDir+"/new/";
	string mailDirCur = tS->mailDir+"/cur/";

		if (mailExists(mailDirCur, name)) {
			deleteFile(mailDirCur+name);
		}
		copyFile(mailDirNew+name, mailDirCur+name+":2,");
		deleteFile(mailDirNew+name);
}


/* List operations over mails */


/*
 * Function cleans the mailList which contains all mails while the list is not empty
 *
 */
void disposeList() {
	while(!mailList.empty()) {
		mailList.pop_back();
	}
}


/*
 * Function inserts mail into list of mails, first of all allocates space for mailStruct structure, then fills it with values and pushes it into the list or with default values (id of mail is current size of list + 1 and toDelete value is always false at the init
 *
 * @param string name name of the mail from Maildir folder
 * @param size_t size size of the mail in octets
 *
 */
void insertMail(string name, size_t size, string dir) {

	auto *mail = new mailStruct;

	mail->id = mailList.size()+1;
	mail->name.assign(name);
	mail->size = size;
	mail->dir.assign(dir);
	mail->toDelete = false;

	mailList.push_back(*mail);
}


/*
 * Function copies the size of mail from list on certain index and stores it in the pointer in the second argument
 *
 * @param unsigned int index index of mail
 * @param size_t *size pointer to the variable where the size will be stored
 *
 */
void copySize(unsigned int index, size_t *size) {
	for (list<mailStruct>::iterator i = mailList.begin(); i != mailList.end(); i++) {
		if (i->id == index) {
			*size = i->size;
			break;
		}
	}
}


/*
 * Function copies the name of mail from list on certain index and stores it in the pointer in the second argument
 *
 * @param unsigned int index index of mail
 * @param string *name pointer to the variable where the name will be stored
 *
 */
void copyName(unsigned int index, string *name) {
	for (list<mailStruct>::iterator i = mailList.begin(); i != mailList.end(); i++) {
		if (i->id == index) {
			*name = i->name;
			break;
		}
	}
}


/*
 * Function copies the deletion flag of mail from list on certain index and stores it in the pointer in the second argument
 *
 * @param unsigned int index index of mail
 * @param bool *toDelete pointer to the variable where the deletion flag will be stored
 *
 */
void copyToDelete(unsigned int index, bool *toDelete) {
	for (list<mailStruct>::iterator i = mailList.begin(); i != mailList.end(); i++) {
		if (i->id == index) {
			*toDelete = i->toDelete;
			break;
		}
	}
}


/*
 * Function copies the deletion flag of mail from list on certain index and stores it in the pointer in the second argument
 *
 * @param unsigned int index index of mail
 * @param string *dir pointer to the variable where path to mail directory will be stored
 *
 */
void copyDir(unsigned int index, string *dir) {
	for (list<mailStruct>::iterator i = mailList.begin(); i != mailList.end(); i++) {
		if (i->id == index) {
			*dir = i->dir;
			break;
		}
	}
}


/*
 * Function sets the deletion flag of mail on certain index
 *
 * @param unsigned int index index of mail
 * @param bool toDelete value of deletion flag that will be saved
 *
 */
void setToDelete(unsigned int index, bool toDelete) {
	for (list<mailStruct>::iterator i = mailList.begin(); i != mailList.end(); i++) {
		if (i->id == index) {
			i->toDelete = toDelete;
			break;
		}
	}
}


/*
 * Function checks the deletion flag of mail on certain index
 *
 * @param unsigned int index index of mail
 * @returns bool value of deletion flag, true if marked for deletion, false otherwise
 */
bool checkIfMarkedForDeletion(unsigned int index) {
	bool toDelete = false;
	copyToDelete(index, &toDelete);
	return toDelete;
}


/*
 * Function returns sum of all mails that are not marked for deletion
 *
 * @returns unsigned int sum of all mails
 */
unsigned int sumOfMails() {
	unsigned int sum = 0;
	for (list<mailStruct>::iterator i = mailList.begin(); i != mailList.end(); i++) {
		if (!i->toDelete) {
			sum++;
		}
	}
	return sum;
}



/*
 * Function returns sum of all mails that are not marked for deletion
 *
 * @returns unsigned int sum of all mails
 */
unsigned int sumOfAllMails() {
	unsigned int sum = 0;
	for (list<mailStruct>::iterator i = mailList.begin(); i != mailList.end(); i++) {
			sum++;
	}
	return sum;
}


/*
 * Function checks if the index of mail given is in the list
 *
 * @param unsigned int index of index of mail
 * @returns bool true if index is in range, false otherwise
 */
bool checkIndexOfMail(unsigned int index) {
	return (index > 0 && index <= sumOfAllMails());
}


/*
 * Functions sums the size of all mails and returns it
 *
 * @returns size_t sum of all mails in octets
 */
size_t sumOfSizeMails() {
	size_t sum = 0;
	for (list<mailStruct>::iterator i = mailList.begin(); i != mailList.end(); i++) {
		if (!i->toDelete) {
			sum += i->size;
		}
	}
	return sum;
}


/*
 * Functions takes content of mail with name and returns lines of content after header
 *
 * @param threadStruct *tS thread structure containing mail directory info and other useful information
 * @param string name name of the mail
 * @param int lines number of lines that are being sent
 *
 */
void sendMail(threadStruct *tS, string name, int lines) {
	string line = "";
	bool header = true;
	int i = 1;

	ifstream file;
	file.open(tS->mailDir+"/cur/"+name);
	while (getline(file, line) && i <= lines) {
		if (line == "" && header) {
			header = false;
			sendMessage(tS->commSocket, line);
			continue;
		}
		if (line != "" && header) {
			sendMessage(tS->commSocket, line);
			continue;
		}
		if (line.find(".") != string::npos && line.find(".") == 0) {
			sendMessage(tS->commSocket, "."+line);
		} else {
			sendMessage(tS->commSocket, line);
		}
		i++;
	}
}


/*
 * Function sends the whole content with header of mail with name
 *
 * @param threadStruct *tS thread structure containing mail directory info and other useful information
 * @param string name name of the mail
 */
void sendMailWithHeader(threadStruct *tS, string name) {
	string line = "";

	ifstream file;
	file.open(tS->mailDir+"/cur/"+name);
	while (getline(file, line)) {
		if (line.find(".") != string::npos && line.find(".") == 0) {
			sendMessage(tS->commSocket, "."+line);
		} else {
			sendMessage(tS->commSocket, line);
		}
	}

	file.close();
}


/*
 * Function closes connection on the socket, sends bye response to client, closes it, unlocks mutex and exits thread
 *
 * @param int socket value of socket that is being closed
 *
 */
void closeConnection(int socket) {
	sendResponse(socket, false, "bye");
	close(socket);
	mutex1.unlock();
}


/*
 * Function checks if the mail directory contains /new and /cur  (/tmp folders - currently disabled)
 *
 * @param string dir name of the mail directory
 * @returns bool true if mail directory is alright, false otherwise
 */
bool checkMailDir(string dir) {
 	return (isDirectory((dir+"/new").c_str()) && isDirectory((dir+"/cur").c_str())/* && isDirectory((dir+"/tmp").c_str())*/);
}


/*
 * Function does reset of the mailList, which means that all mails marked for deletion will be unmarked
 *
 * @param threadStruct *tS thread structure containing mail directory info and other useful information
 *
 */
void rsetOperation(threadStruct *tS) {
	for (unsigned int i = 1; i <= mailList.size(); i++) {
		setToDelete(i, false);
	}
	sendResponse(tS->commSocket, false, "user's maildrop has "+to_string(sumOfMails())+" messages ("+to_string(sumOfSizeMails())+") octets");
}


/*
 * Function does reset of the mailList, which means that all mails marked for deletion will be unmarked
 *
 * @param threadStruct *tS thread structure containing mail directory info and other useful information
 * @param unsigned int index index of mail
 *
 */
void deleOperation(threadStruct *tS, unsigned int index) {
	if (!checkIndexOfMail(index)) {
		sendResponse(tS->commSocket, true, "no such message (only "+to_string(sumOfAllMails())+" messages in maildrop)");
	} else {
		if (checkIfMarkedForDeletion(index)) {
			sendResponse(tS->commSocket, true, "mail already marked for deletion");
		} else {
			setToDelete(index, true);
			sendResponse(tS->commSocket, false, "mail marked for deletion");
		}
	}
}


/*
 * Function sends a client status info containing how many mails are in mail directory and what is their size
 *
 * @param threadStruct *tS thread structure containing mail directory info and other useful information
 *
 */
void statOperation(threadStruct *tS) {
	sendResponse(tS->commSocket, false, to_string(sumOfMails())+" "+to_string(sumOfSizeMails()));
}


/*
 * Function does just nothing, typical NOOP, it is here just so the application looks nice and that every operation has its own function, sends blank response to client
 *
 * @param threadStruct *tS thread structure containing mail directory info and other useful information
 *
 */
void noopOperation(threadStruct *tS) {
	sendResponse(tS->commSocket, false, "");
}


/*
 * Function returns the params of mail on certain index
 *
 * @param threadStruct *tS thread structure containing mail directory info and other useful information
 * @param unsigned int index index of mail we are looking for
 *
 */
void listIndexOperation(threadStruct *tS, unsigned int index) {
	if (!checkIndexOfMail(index)) {
		sendResponse(tS->commSocket, true, "no such message (only "+to_string(sumOfMails())+" messages in maildrop)");
	} else {
		if (checkIfMarkedForDeletion(index)) {
			sendResponse(tS->commSocket, true, "mail marked for deletion");
		} else {
			size_t size;
			copySize(index, &size);
			sendResponse(tS->commSocket, false, to_string(index)+" "+to_string(size));
		}
	}

}


/*
 * Function returns info about the email almost the same as the list, but it also returns the content of mail including its header
 *
 * @param threadStruct *tS thread structure containing mail directory info and other useful information
 * @param unsigned int index index of mail we are looking for
 *
 */
void retrOperation(threadStruct *tS, unsigned int index) {
	if (!checkIndexOfMail(index)) {
		sendResponse(tS->commSocket, true, "no such message (only "+to_string(sumOfMails())+" messages in maildrop)");
	} else {
		if (checkIfMarkedForDeletion(index)) {
			sendResponse(tS->commSocket, true, "mail marked for deletion");
		} else {
			listIndexOperation(tS, index);
			string name;
			copyName(index, &name);
			sendMailWithHeader(tS, name);
			sendMessage(tS->commSocket, ".");
		}
	}

}


/*
 * Function takes all e-mails, sums nondeleted and sums their size and sends it to the client, after that, the server sends size of every single messages one by one
 *
 * @param threadStruct *tS thread structure containing mail directory info and other useful information
 *
 */
void listOperation(threadStruct *tS) {
	sendResponse(tS->commSocket, false, ""+to_string(sumOfMails())+" messages ("+to_string(sumOfSizeMails())+" octets)");

	// take mails one by one and send them
	unsigned int index = 1;
	while (index <= sumOfAllMails()) {
		if (!checkIfMarkedForDeletion(index)) {
			size_t size;
			copySize(index, &size);
			sendMessage(tS->commSocket, to_string(index)+" "+to_string(size));
		}
		index++;
	}
	sendMessage(tS->commSocket, ".");
}


/*
 * Function creates a unique md5 hash name consisting of mail directory, cur folder and mailname
 *
 * @param threadStruct *tS thread structure containing mail directory info and other useful information
 * @param string mailName name of the mail that is being hashed
 * @returns string unique md5 hash hash for any mail in the system
 */
string hashForUidl(string dir, string mailName, size_t size) {
	return md5(dir+"/cur/"+mailName+to_string(size));
}


/*
 * Function generates a unique md5 hash for every mail in mail directory and sends it to the client
 *
 * @param threadStruct *tS thread structure containing mail directory info and other useful information
 *
 */
void uidlOperation(threadStruct *tS) {
	sendResponse(tS->commSocket, false, to_string(sumOfMails())+" messages in maildrop");
	if (sumOfMails() > 0) {
		// take mails one by one, hash them and send them with id
		unsigned int index = 1;
		while (index <= sumOfAllMails()) {
			if (!checkIfMarkedForDeletion(index)) {
				string mailName;
				string dir;
				size_t size;
				copyName(index, &mailName);
				copyDir(index, &dir);
				copySize(index, &size);
				sendResponse(tS->commSocket, false, to_string(index)+" "+hashForUidl(dir, mailName, size));
			}
			index++;
		}
	}
	sendMessage(tS->commSocket, ".");
}


/*
 * Function generates a unique md5 hash for mail on index given in second parameter in mail directory and sends it to the client
 *
 * @param threadStruct *tS thread structure containing mail directory info and other useful information
 * @param unsigned int index index of a mail
 *
 */
void uidlIndexOperation(threadStruct *tS, unsigned int index) {
	if (!checkIndexOfMail(index)) {
		sendResponse(tS->commSocket, true, "no such message (only "+to_string(sumOfMails())+" messages in maildrop)");
	} else {
		if (!checkIfMarkedForDeletion(index)) {
			string mailName;
			string dir;
			size_t size;
			copyName(index, &mailName);
			copyDir(index, &dir);
			copySize(index, &size);
			sendResponse(tS->commSocket, false, to_string(index)+" "+hashForUidl(dir, mailName, size));
		} else {
			sendResponse(tS->commSocket, true, "mail does not exist");
		}
	}
}


void deleteMarkedForDeletion(threadStruct *tS, int *errors) {
	unsigned int i = 0;
	while (i <= sumOfMails()) {
		if (checkIfMarkedForDeletion(i)) {
			string dir;
			string name;
			copyDir(i, &dir);
			copyName(i, &name);
			if (!deleteFile(dir+"/cur/"+name)) {
				*errors += 1;
			}
		}
		i++;
	}
}


/*
 * Function takes the body of the e-mail (without header) and returns rows of lines
 *
 * @param threadStruct *tS thread structure containing mail directory info and other useful information
 * @param unsigned int index index of a mail
 * @param int rows number of rows requested from mail
 *
 */
void topIndexOperation(threadStruct *tS, unsigned int index, int rows) {
	if (!checkIndexOfMail(index)) {
		sendResponse(tS->commSocket, true, "no such message (only "+to_string(sumOfMails())+" messages in maildrop)");
	} else {
		if (!checkIfMarkedForDeletion(index)) {
			string name;
			copyName(index, &name);
			sendResponse(tS->commSocket, false, "");
			sendMail(tS, name, rows);
			sendMessage(tS->commSocket, ".");
		} else {
			sendResponse(tS->commSocket, true, "mail does not exist");
		}
	}
}


/*
 * TODO i should close current thread there and remove it from vector
 */
void closeThread(threadStruct *tS) {
	int i = 0;
	for(std::vector<threadStruct>::iterator it = threads.begin(); it != threads.end(); ++it) {
		if (it->commSocket == tS->commSocket) {

		}
		i++;
	}
}

/*
 * TODO dont know how to close single thread and delete it from vector...
 */
void quitOperation(threadStruct *tS) {
	int errors = 0;
	deleteMarkedForDeletion(tS, &errors);
	if (errors) {
		sendResponse(tS->commSocket, true, "some deleted messages not removed");
	} else {
		if (sumOfMails() != 1) {
			sendResponse(tS->commSocket, false, tS->serverUser+" POP3 server signing off, "+to_string(sumOfMails())+" messages left");
		} else {
			sendResponse(tS->commSocket, false, tS->serverUser+" POP3 server signing off, "+to_string(sumOfMails())+" message left");
		}

	}
	closeConnection(tS->commSocket);
	closeThread(tS);
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
	size_t size = (unsigned) fileStream.tellg();
	fileStream.seekg(0);

	stringstream buffer;
	buffer << fileStream.rdbuf();
	char string[buffer.str().length()];
	strcpy(string, buffer.str().c_str());
	for (unsigned int i = 0; i < strlen(string); i++) {
		if(string[i] == '\n' && string[i-1] != '\r') {
			size++;
		}
		/*if() {
			size++;
		}*/
	}

	fileStream.close();
	return size;
}



void loadMailsFromCfg(threadStruct *tS) {


	string line;

	ifstream config ((cwd+"/"+mailConfig).c_str());
	if (config.is_open()) {
		int i = 0;
		string dir;
		string name;
		while(getline(config, line)) {
			if (i % 2 == 0) {
				dir = returnSubstring(line, "dir = ", true);
			} else {
				name = returnSubstring(line, "name = ", true);
				if (fileExists(string(dir+"/cur/"+name).c_str())) {
					insertMail(name, getFileSize(dir+"/cur/"+name), dir);
				}
			}
			i++;
		}
		deleteFile(cwd+"/"+mailConfig);
	}
}

/*
 * Function gets all emails from mail directory and fills them into the global list of mails
 *
 * @param threadStruct *tS thread structure containing mail directory info and other useful information
 *
 */
void createListFromMails(threadStruct *tS) {


	loadMailsFromCfg(tS);

	string name = "";
	DIR *dir;
	struct dirent *ent;

	if ((dir = opendir((tS->mailDir+"/new/").c_str())) != nullptr) {
		while ((ent = readdir(dir)) != nullptr) {
			if (string(ent->d_name) == ".." || string(ent->d_name) == ".") {
				continue;
			} else {
				name = ent->d_name;
				insertMail(name+":2,", getFileSize(tS->mailDir+"/new/"+ent->d_name), tS->mailDir);
				moveNewToCur(tS, name);
			}
		}
	}
}


/*
 * Function validates a request depending on operation
 *
 * @param string received message that is being validated
 * @param int operation operation that should be processed
 * @returns bool true if request and its operands were ok, false otherwise
 */
bool validateRequest(string received, int operation) {

	string parsed;
	// top should have 2 arguments, uidlIndex, dele, retr and listIndex should have 1, all other should have 0 - those that have 0 arguments are already checked in getOperation function
	if (operation == 14) {
		// TOP 3 2\r\n
		if ((parsed = returnSubstring(received, "\r\n", false)) != "") {
			// TOP 3 2
			if ((parsed = returnSubstring(parsed, " ", true)) != "") {
				// 3 2
				string a = returnSubstring(parsed, " ", false);
				string b = returnSubstring(parsed, " ", true);
				try {
					stoi(a);
				} catch(...) {
					return false;
				}
				try {
					stoi(b);
				} catch(...) {
					return false;
				}
			} else {
				return false;
			}
		} else {
			return false;
		}
	} else if (operation == 13 || operation == 10 || operation == 9 || operation == 6) {
		// [UIDL|DELE|RETR|LIST] 3\r\n
		if ((parsed = returnSubstring(received, "\r\n", false)) != "") {
			// [UIDL|DELE|RETR|LIST] 3
			if ((parsed = returnSubstring(parsed, " ", true)) != "") {
				// 3
				try {
					stoi(parsed);
				} catch(...) {
					return false;
				}
			} else {
				return false;
			}
		} else {
			return false;
		}
	} else {
		// already checked in getOperation()
	}

	return true;
}


/*
 * Function executes the mail servers, first of all it tries to lock the mail directory by mutex, if the lock is successful, all mails from new are moved to cur, list from mails is created. After that, server responds to clients with amount of mails and their complete size. After the "init" the server waits for requests and works on these operations.
 *
 * @param threadStruct *tS thread structure containing mail directory info and other useful information
 * @param int op integer value of operation
 * @param string received message that was received will be validated and parsed
 *
 */
void executeMailServer(threadStruct *tS, int op, string received) {
	if (op > 0) {
		if (validateRequest(received, op)) {
			if (op > 0 && op < 4) {
				sendResponse(tS->commSocket, true, "already authorized");
			} else if (op == 4) {
				quitOperation(tS);
				closeConnection(tS->commSocket);
			} else if (op == 5) {
				listOperation(tS);
			} else if (op == 6) {
				listIndexOperation(tS, (unsigned) stoi(returnSubstring(returnSubstring(string(received), "\r\n", false), " ", true), nullptr));
			} else if (op == 7) {
				noopOperation(tS);
			} else if (op == 8) {
				statOperation(tS);
			} else if (op == 9) {
				retrOperation(tS, (unsigned) stoi(returnSubstring(returnSubstring(string(received), "\r\n", false), " ", true), nullptr));
			} else if (op == 10) {
				deleOperation(tS, (unsigned) stoi(returnSubstring(returnSubstring(string(received), "\r\n", false), " ", true), nullptr));
			} else if (op == 11) {
				rsetOperation(tS);
			} else if (op == 12) {
				uidlOperation(tS);
			} else if (op == 13) {
				uidlIndexOperation(tS, (unsigned) stoi(returnSubstring(returnSubstring(string(received), "\r\n", false), " ", true), nullptr));
			} else if (op == 14) {
				topIndexOperation(tS, (unsigned) stoi(returnSubstring(returnSubstring(returnSubstring(string(received), "\r\n", false), " ", true), " ", false), nullptr), stoi(returnSubstring(returnSubstring(returnSubstring(string(received), "\r\n", false), " ", true), " ", true), nullptr));
			} else {
				sendResponse(tS->commSocket, true, "invalid command");
			}
		} else {
			sendResponse(tS->commSocket, true, "invalid command");
		}
	} else {
		sendResponse(tS->commSocket, true, "invalid command");
	}
}


/*
 * Function locks the mail directory for current user, if the mail directory is already used, it won't allow usage for another user
 *
 * @param threadStruct *tS thread structure containing mail directory info and other useful information
 * @returns bool true if mail directory was locked, false if otherwise
 */
bool lockMaildir(threadStruct *tS) {

	if (mutex1.try_lock()) {
		if (!checkMailDir(tS->mailDir)) {
			sendResponse(tS->commSocket, true, "mail directory not OK");
			closeConnection(tS->commSocket);
			return false;
		}
	} else {
		sendResponse(tS->commSocket, true, "permission denied");
		return false;
	}

	createListFromMails(tS);

	sendResponse(tS->commSocket, false, "user's maildrop has "+to_string(sumOfMails())+" messages ("+to_string(sumOfSizeMails())+" octets)");
	return true;
}

/*
 * Function authorizes user depending on his credentials. It selects between two modes, hashed and non-hashed mode
 * If the param -c was passed to the program, it will accept only non-hashed version of password, otherwise it accepts only hashed version of password
 * Hashing is done on the client side using md5 depending on the pidTimeStamp that the server sends
 *
 * @param int op value of operation to check correct order
 * @param threadStruct *tS thread structure containing mail directory info and other useful information
 * @param char *receivedMessage message that is being parsed and from which the authorization data are taken
 * @param bool isHashed param that selects mode of authorization (hashed/non-hashed)
 * @returns bool true if user is authorized correctly, false otherwise
 */
bool authorizeUser(int op, threadStruct *tS, char *receivedMessage, bool isHashed) {
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
						sendResponse(tS->commSocket, true, "invalid command");
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
			sendResponse(tS->commSocket, true, "invalid command");
			return false;
		}
	} else {
		if (op == 1) {
			if (checkUser(tS->clientUser = returnSubstring(returnSubstring(receivedMessage, " ", true), " ", false), tS->serverUser)) {
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
			sendResponse(tS->commSocket, true, "invalid command");
			return false;
		}

	}
	return lockMaildir(tS);
}


/*
 * Function serves as a client thread operation. Every client that connects to the server is given a separate thread, function authorizes user and calls mail server execution function
 *
 * @param void *tS threadStruct given by void, later on retyped
 * @return nullptr
 */
void *clientThread(void *tS) {

	char receivedMessage[1024];
	auto *tParam = (threadStruct *) tS;

	threads.push_back(*tParam);

	tParam->pidTimeStamp = generatePidTimeStamp();
	clock_t clock1 = clock();
	double timeout = 10;

	sendResponse(tParam->commSocket, false, "POP3 server ready "+tParam->pidTimeStamp);
	for (;;) {
		if ((double) ((clock() - clock1) / CLOCKS_PER_SEC) <= timeout) {
			//cout << ((double) clock() - clock1 / CLOCKS_PER_SEC) << endl;
			if (((int) recv(tParam->commSocket, receivedMessage, 1024, 0)) <= 0) {
				break;
			} else {
				clock1 = clock();
				int op = 0;
				if ((op = getOperation(receivedMessage)) != 0) {
					clock1 = clock();
					if (tParam->authorized) {
						executeMailServer(tParam, op, receivedMessage);
					} else {
						tParam->authorized = authorizeUser(op, tParam, receivedMessage, tParam->isHashed);
					}
				} else {
					sendResponse(tParam->commSocket, true, "invalid command");
				}
			}
		} else {
			sendMessage(tParam->commSocket, "You have been logged out after "+to_string(timeout)+" seconds.");
			closeThread(tParam);
		}
	}
	return nullptr;
}


/*
 * Function resets mail directory to default settings - moves all mails from cur to previous folders, does not work with deleted mails
 *
 */
void resetMail() {

	string line;

	ifstream config ((cwd+"/"+mailConfig).c_str());
	if (config.is_open()) {
		int i = 0;
		string dir;
		string name;
		while(getline(config, line)) {
			if (i % 2 == 0) {
				dir = returnSubstring(line, "dir = ", true);
			} else {
				name = returnSubstring(line, "name = ", true);
				copyFile(dir+"/cur/"+name, dir+"/new/"+returnSubstring(name, ":2,", false));
				deleteFile(dir+"/cur/"+name);
			}
			i++;
		}
	deleteFile(cwd+"/"+mailConfig);
	}
}


/*
 * Function creates a mail config, that is saved in the same folder as the server, this function is fundamental for -reset param
 * Format of this config is following:
 * maildirdir = dir
 * name = name:2, (:2, is already in name); name = returnSubstring(name, ":2,", false)
 */
void createMailCfg() {

	string name;
	string dir;
	ofstream file;
	file.open(cwd+"/"+mailConfig);

	unsigned int i = 1;
	while (i <= mailList.size()) {
		bool toDelete;
		copyToDelete(i,&toDelete);
		if (!toDelete) {
			copyName(i, &name);
			copyDir(i, &dir);
			file << "dir = " << dir << endl;
			file << "name = " << name << endl;
		}
		i++;
	}
	file.close();
}


/*
 * Function takes all threads and closes them one by one from the last to the first
 *
 */
void closeThreads() {
	while(!threads.empty()) {
		closeConnection(threads.back().commSocket);
		threads.pop_back();
	}
}


/*
 * Function checks whether sigint was passed, if yes, the server correctly ends its process
 *
 * @param int param not used
 */
void sigintHandler(int param) {
	createMailCfg();
	disposeList();
	closeThreads();
	exit(EXIT_SUCCESS);
}


/*
 *
 */
int main(int argc, char *argv[]) {

	signal(SIGINT, sigintHandler);

	bool help = false;
	bool reset = false;
	string mailDir;
	string usersFile;
	string serverUser;
	string serverPass;
	int port = 0;
	bool isHashed = true;

	// params
	if (!checkParams(argc) || !parseParams(argc, argv, help, isHashed, reset, usersFile, mailDir, port)) {
		throwException("ERROR: Wrong arguments.");
	}

	cwd = getWorkindDirectory();
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

	socklen_t clientlen = sizeof(clientaddr);

	// server started perfectly, now creating a cycle for server behaviour
	while (true) {
		int commSocket;
		if ((commSocket = accept(serverSocket, (struct sockaddr *) &clientaddr, &clientlen)) > 0) {
			pthread_t thread;
			auto *tS = new threadStruct;
			tS->serverUser = serverUser;
			tS->serverPass = serverPass;
			tS->isHashed = isHashed;
			tS->mailDir= mailDir;
			tS->commSocket = commSocket;
			pthread_create(&thread, nullptr, clientThread, tS);
		} else {
			break;
		}
	}

	return EXIT_SUCCESS;
}
