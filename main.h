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
void printHelp();
bool checkParams(int argc);
bool parseParams(int argc, char *argv[], bool &help, bool &isHashed, bool &reset, string &usersFile, string &mailDir, int &port);
string returnSubstring(string String, string delimiter, bool way);
bool checkUsersFile(const char *name, string &user, string &pass);
int getOperation(string message);
string generatePidTimeStamp();
string getWorkingDirectory(char *argv[]);
bool isPidUnique(string pidTimeStamp);
void sendResponse(int socket, bool error, string message);
void sendMessage(int socket, string message);
bool checkUser(string clientUser, string serverUser);
bool authenticateUser(string clientPass, string serverPass);
void copyFile(string from, string to);
bool mailExists(string dir, string name);
bool deleteFile(string path);
void moveNewToCur(threadStruct *tS, string name);
void closeConnection(int socket);
bool checkMailDir(string dir);
void rsetOperation(threadStruct *tS);
void deleOperation(threadStruct *tS, unsigned int index);
void statOperation(threadStruct *tS);
void noopOperation(threadStruct *tS);
void listIndexOperation(threadStruct *tS, unsigned int index);
void retrOperation(threadStruct *tS, unsigned int index);
void listOperation(threadStruct *tS);
string hashForUidl(string dir, string mailName, size_t size);
void uidlOperation(threadStruct *tS);
void uidlIndexOperation(threadStruct *tS, unsigned int index);
void deleteMarkedForDeletion(threadStruct *tS, int *errors);
void topIndexOperation(threadStruct *tS, unsigned int index, int rows);
void closeThread(threadStruct *tS);
void quitOperation(threadStruct *tS);
size_t getFileSize(string file);
void loadMailsFromCfg(threadStruct *tS);
void createListFromMails(threadStruct *tS);
bool validateRequest(string received, int operation);
void executeMailServer(threadStruct *tS, int op, string received);
bool lockMaildir(threadStruct *tS);
bool authorizeUser(int op, threadStruct *tS, char *receivedMessage, bool isHashed);
void *clientThread(void *tS);
void resetMail();
void createMailCfg();
void closeThreads();
void sigintHandler(int param);

/* Mail backend operations */
void disposeList();
void insertMail(string name, size_t size, string dir);
void copySize(unsigned int index, size_t *size);
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


#endif //POP3_MAIN_H
