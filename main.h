//
// Created by skalin on 18.11.17.
//

#ifndef POP3_MAIN_H
#define POP3_MAIN_H


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


#endif //POP3_MAIN_H
