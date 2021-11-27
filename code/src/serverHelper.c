#include "../header/serverHelper.h"

void printUsage(){
    fprintf(stdout, "./twmailer-server <port> <mail-spool-directory>\n"); 
}

void signalHandler(int signal){
    if(signal == SIGINT){
        fprintf(stdout, "Server shutdown requested\n"); 
        abortRequested = 1; 
        if(newSocket != -1){
            if(shutdown(newSocket, SHUT_RDWR) == -1)
                perror("SHUTDOWN error: newSocket");
            if(close(newSocket) == -1){
                perror("CLOSE error: newSocket");
            }
            serverSocket = -1; 
        }
        if(serverSocket != -1){
            if(shutdown(serverSocket, SHUT_RDWR) == -1)
                perror("SHUTDOWN error: serverSocket");
            if(close(serverSocket) == -1){
                perror("CLOSE error: serverSocket");
            }
            serverSocket = -1; 
        }   
    }
}

void sendFeedback(int socket, char* feedback){
    if(send(socket, feedback, strlen(feedback), 0) == -1){
        perror("Server failed to send answer"); 
        return; 
    }
}
