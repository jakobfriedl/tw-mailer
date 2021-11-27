#ifndef HELPER_H
#define HELPER_H

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>

#define BUFFER 1024
#define SUBJECT_LENGTH 80
#define MAX_USERNAME_LENGTH 8
#define MIN_USERNAME_LENGTH 1
#ifndef SO_REUSEPORT
    #define SO_REUSEPORT 15  
#endif
typedef struct mail{
    char sender[BUFFER];
    char receiver[BUFFER];
    char subject[BUFFER]; 
    char message[BUFFER];   
} mail_t; 

ssize_t my_read(int socketDecriptor, char *ptr); 
ssize_t readline(int socketDecriptor, void *buffer, size_t maxlen);
ssize_t writen(int socketDescriptor, const void *buffer, size_t n); 
int validateUserName(char* username); 
int sendData(int socket, char* buffer, int bytesToSend); 

#endif