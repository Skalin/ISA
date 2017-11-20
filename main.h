//
// Created by Dominik Skála on 18.11.17.
//

#ifndef POP3_MAIN_H
#define POP3_MAIN_H

#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <mutex>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <sstream>
#include <fstream>
#include <csignal>
#include <vector>
#include <list>
#include <algorithm>


using namespace std;


struct mailStruct{
	unsigned long id;
	string name;
	size_t size;
	string dir;
	bool toDelete;
} *mailStructPtr;



typedef struct {
	string mailDir = "";
	string usersFile = "";
	bool isHashed = true;
	string clientUser = "";
	string serverUser = "";
	string clientPass = "";
	string serverPass = "";
	int commSocket = -1;
	string pidTimeStamp = "";
	bool authorized = false;
} threadStruct;


void throwException(const char *message);
bool fileExists(const char *file);
bool isDirectory(const char *folder);
string getStringTime();
void printHelp();
bool parseParams(int argc, char *argv[], bool &help, bool &isHashed, bool &reset, string &usersFile, string &mailDir, int &port);
string returnSubstring(string String, string delimiter, bool way);
string getWorkingDirectory(char *argv[]);
void copyFile(string from, string to);
bool deleteFile(string path);

/* Server functions */
void *clientThread(void *tS);
void sendResponse(int socket, bool error, string message);
void sendMessage(int socket, string message);
void closeConnection(int socket);
void closeThreads();
void sigintHandler(int param);

/* POP3 functions */
void rsetOperation(threadStruct *tS);
void deleOperation(threadStruct *tS, unsigned int index);
void statOperation(threadStruct *tS);
void noopOperation(threadStruct *tS);
void listIndexOperation(threadStruct *tS, unsigned int index);
void retrOperation(threadStruct *tS, unsigned int index);
void listOperation(threadStruct *tS);
void uidlOperation(threadStruct *tS);
void uidlIndexOperation(threadStruct *tS, unsigned int index);
void deleteMarkedForDeletion(int *errors);
void topIndexOperation(threadStruct *tS, unsigned int index, int rows);
void quitOperation(threadStruct *tS);

/* Mail backend operations */
bool checkUsersFile(const char *name, string &user, string &pass);
bool checkMailDir(string dir);
int getOperation(string message);
string generatePidTimeStamp();
bool mailExists(string dir, string name);
void moveNewToCur(threadStruct *tS, string name);
bool checkUser(string clientUser, string serverUser);
bool authenticateUser(string clientPass, string serverPass);
bool lockMaildir(threadStruct *tS);
void executeMailServer(threadStruct *tS, int op, string received);
bool validateRequest(string received, int operation);
bool authorizeUser(int op, threadStruct *tS, char *receivedMessage, bool isHashed);
void createListFromMails(threadStruct *tS);
void disposeList();
void insertMail(string name, size_t size, string dir);
void copySize(unsigned int index, size_t *size);
size_t getFileSize(string file);
void copyName(unsigned int index, string *name);
void copyToDelete(unsigned int index, bool *toDelete);
void copyDir(unsigned int index, string *dir);
void setToDelete(unsigned int index, bool toDelete);
bool checkIfMarkedForDeletion(unsigned int index);
unsigned int sumOfMails();
unsigned int sumOfAllMails();
bool checkIndexOfMail(unsigned int index);
size_t sumOfSizeMails();
void sendMail(threadStruct *tS, string name, int lines);
void sendMailWithHeader(threadStruct *tS, string name);
string hashForUidl(threadStruct *tS, string dir, string mailName);
void loadMailsFromCfg();
void resetMail();
void createMailCfg();

#endif //POP3_MAIN_H
