#ifndef SERVER_COMMANDS_H
#define SERVER_COMMANDS_H

#include "serverHelper.h"

int handleLoginRequest(int socket, char* sessionUser, char* ip); 
int handleSendRequest(int socket, char* sessionUser); 
int handleListRequest(int socket, char* sessionUser); 
int handleReadRequest(int socket, char* sessionUser); 
int handleDelRequest(int socket, char* sessionUser); 

#endif