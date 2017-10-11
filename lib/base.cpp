//
// Created by Dominik on 10/9/2017.
//



#include "base.h"




/*
 * Function prints a error message on cerr and exits program
 *
 * @param const char *message message to be printed to cerr
 */
void throwException(const char *message) {
	cerr << (message) << endl;
	exit(EXIT_FAILURE);
}


bool checkParams(int argc) {
	if (argc != 2 && (argc < 4 || argc > 6)) {
		throwException("ERROR: Wrong amount of arguments.");
	}
	
	return true;
}

bool parseParams(char *argv[]) {
	
	return true;
}
