#ifndef SERVER_HELPER_H
#define SERVER_HELPER_H

#include "helper.h"
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <ldap.h>
#include <time.h>

// Global Variables
char mail_spool[PATH_MAX]; 
int abortRequested; 
int serverSocket; 
int newSocket;
struct stat st;  

// Threading
#define NUM_CLIENTS 100
pthread_mutex_t mutex;
pthread_t clients[NUM_CLIENTS];

typedef struct args{
    int socket; 
    char* ip; 
}args_t;

// Functions
void printUsage(); 
void signalHandler(int signal); 
void sendFeedback(int socket, char* feedback); 

#endif