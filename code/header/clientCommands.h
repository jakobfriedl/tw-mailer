#ifndef CLIENT_COMMANDS_H
#define CLIENT_COMMANDS_H

#include "helper.h"
#include "clientHelper.h"

int sendLoginRequest(int socket);
int sendSendRequest(int socket);
int sendListRequest(int socket);
int sendReadRequest(int socket);
int sendDelRequest(int socket);

#endif