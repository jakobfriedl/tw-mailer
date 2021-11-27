#ifndef CLIENT_HELPER_H
#define CLIENT_HELPER_H

#include "helper.h"
#include <termios.h>

void printUsage();
int receiveFeedback(int socket); 
void getPassword(char* pw);
int sendData(int socket, char* buffer, int bytesToSend); 

#endif