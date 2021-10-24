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
#define SUBJECT_LENGTH 81
#ifndef SO_REUSEPORT
    #define SO_REUSEPORT 15  
#endif

typedef struct mail{
    char sender[BUFFER];
    char receiver[BUFFER];
    char subject[SUBJECT_LENGTH]; 
    char message[BUFFER];  
} mail_t; 

static ssize_t my_read (int socketDescriptor, char *ptr); 
ssize_t readline(int socketDescriptor, void* buffer, size_t bufferLength);
ssize_t writen(int socketDescriptor, const void *buffer, size_t n); 


static ssize_t my_read(int socketDescriptor, char *ptr) { 
    static int read_cnt = 0; 
    static char *read_ptr; 
    static char buffer[BUFFER]; 
    
    if(read_cnt <= 0){ 
        again: 
            if((read_cnt = read(socketDescriptor, buffer, sizeof(buffer))) < 0){ 
                if (errno == EINTR) 
                    goto again; 
            return -1; 
            } else if (read_cnt == 0) 
                return 0; 
            read_ptr = buffer; 
    } 
    read_cnt--; 
    *ptr = *read_ptr++; 
    return 1; 
}

ssize_t readline(int socketDescriptor, void* buffer, size_t bufferLength){
    ssize_t n, bytesRead; 
    char c, *ptr; 
    ptr = buffer; 
    
    for (n = 1 ; n < bufferLength ; n++) { 
        if((bytesRead = my_read(socketDescriptor, &c)) == 1 ){ 
            *ptr++ = c; 
            if (c == '\n') 
                break; // newline is stored 
        } else if (bytesRead == 0) { 
            if (n == 1) 
                return 0; // EOF, no data read  
            else 
                break; // EOF, some data was read 
        } else 
            return -1; // error, errno set by read() in my_read()    
    } 
    *ptr = 0; // null terminate 
    return n;
}

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