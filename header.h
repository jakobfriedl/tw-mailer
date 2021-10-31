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
#include <regex.h>

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

ssize_t writen(int socketDescriptor, const void *buffer, size_t n); 
int validateUserName(char* username); 

ssize_t writen(int socketDescriptor, const void *buffer, size_t n){
    size_t bytesLeft;
    ssize_t bytesWritten;

    const char *ptr;
    ptr = buffer;

    bytesLeft = n;
    while (bytesLeft > 0){
        if ((bytesWritten = write(socketDescriptor, ptr, bytesLeft)) <= 0){
            if (errno == EINTR)
                bytesWritten = 0; // and call write() again
            else
                return -1;
        }
        bytesLeft -= bytesWritten;
        ptr += bytesWritten;
    }
    return n;
}

int sendData(int socket, char* buffer, int bytesToSend){
    fgets(buffer, bytesToSend-1, stdin); 
    int size = (int)strlen(buffer); 
    if(buffer[size-2] == '\r' && buffer[size-1] == '\n'){
        buffer[size] = 0; 
        size -= 2; 
    }else if(buffer[size-1] == '\n'){
        buffer[size] = 0; 
        --size;
    }
    buffer[size] = '\0';
 
    return writen(socket, buffer, bytesToSend-1);  
}

int validateUserName(char* username){
    // Check if username is max. 8 characters long
    if(strlen(username) > MAX_USERNAME_LENGTH || strlen(username) < MIN_USERNAME_LENGTH){
        return 0; 
    }

    // Check if username only contains letters and numbers
    for(int i = 0; i <= strlen(username)-1; i++){
        if((username[i] >= 'a' && username[i] <= 'z')
        || (username[i] >= 'A' && username[i] <= 'Z')
        || (username[i] >= '0' && username[i] <= '9')){
            // Skip character
        }else{
            return 0; 
        }
    }
    return 1; 
}
