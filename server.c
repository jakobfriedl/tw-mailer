#include "header.h"

int serverSocket = -1; 
int newSocket = -1; 
int abortRequested = 0; 

void printUsage(); 
void signalHandler(int signal); 
void* clientCommunication(void* data); 

int main(int argc, char** argv){
    socklen_t addressLength;
    struct sockaddr_in address, clientAddress; 
    int reuseValue = 1; 

    if(argc != 3){
        printUsage();
        return EXIT_FAILURE; 
    }

    // Signal Handler
    if(signal(SIGINT, signalHandler) == SIG_ERR){
        perror("Signal cannot be registered."); 
        return EXIT_FAILURE; 
    }

    // Create Socket
    if((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("CREATE error"); 
        return EXIT_FAILURE; 
    }

    // Set Socket Options
    if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuseValue, sizeof(reuseValue)) == -1){
        perror("Socket Options - REUSEADDR"); 
    }
    if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &reuseValue, sizeof(reuseValue)) == -1){
        perror("Socket Options - REUSEPORT"); 
    }

    // Initialize Address
    memset(&address, 0, sizeof(address)); 
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = htonl(INADDR_ANY); 
    address.sin_port = htons(atoi(argv[1])); 

    // Assign address & port to socket
    if(bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) == -1){
        perror("BIND error"); 
        return EXIT_FAILURE; 
    }

    // Allow Connection
    if(listen(serverSocket, 5) == -1){
        perror("LISTEN error"); 
        return EXIT_FAILURE;
    }

    while(!abortRequested){
        fprintf(stdout, "Waiting for connection...\n"); 

        // Accept Connection Setup
        addressLength = sizeof(struct sockaddr_in); 
        if((newSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &addressLength)) == -1){
            if(abortRequested){
                perror("ACCEPT error after abort");
            }else{
                perror("ACCEPT error"); 
            } 
            break; 
        }

        // Start Client
        fprintf(stdout, "Client connected from %s:%d...\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
        
        clientCommunication(&newSocket); 

        newSocket = -1; 
    }

    // Close Socket
    if(serverSocket != -1){
        if(shutdown(serverSocket, SHUT_RDWR) == -1)
            perror("SHUTDOWN error: serverSocket");
        if(close(serverSocket) == -1){
            perror("CLOSE error: serverSocket");
        }
        serverSocket = -1; 
    }

    return EXIT_SUCCESS; 
}

void* clientCommunication(void* data){
    char buffer[BUFFER]; 
    int size; 
    int* currentClientSocket = (int *)data; 

    // Welcome Client
    strcpy(buffer, "\nWelcome to TW-Mail-Server, you can use to following commands: \
                    \n   SEND \
                    \n   LIST \
                    \n   READ \
                    \n   DEL \
                    \n   QUIT\n");
    if (send(*currentClientSocket, buffer, strlen(buffer), 0) == -1){
        perror("SEND error");
        return NULL;
    }

    do{
        // Receive Command
        size = recv(*currentClientSocket, buffer, BUFFER-1, 0);
        if(size == -1){
            if(abortRequested){
                perror("RECV error after abort"); 
            }else{ 
                perror("RECV error"); 
            }
            break; 
        }
        if(size == 0){
            printf("Client closed remote socket\n"); 
            break; 
        }

        if(buffer[size-2] == '\r' && buffer[size-1] == '\n')
            size -= 2; 
        else if(buffer[size-1] == '\n')
            --size;            
        buffer[size] = '\0'; // Terminate String

        if(strcmp(buffer, "SEND") == 0){
            printf("SEND COMMAND RECEIVED\n");
            
            mail_t* newMail = (mail_t*)malloc(sizeof(mail_t));

            // Receive Sender
            if(recv(*currentClientSocket, newMail->sender, sizeof(newMail->sender)-1, 0) == -1){
                perror("RECV SENDER error");
            } else {
                printf("Sender: %s: %d\n", newMail->sender, (int)strlen(newMail->sender)); 
            }

            // Receive Receiver
            if(recv(*currentClientSocket, newMail->receiver, sizeof(newMail->receiver)-1, 0) == -1){
                perror("RECV RECEIVER error");
            } else {
                printf("Receiver: %s: %d\n", newMail->receiver, (int)strlen(newMail->receiver)); 
            }

            // Receive Receiver
            if(recv(*currentClientSocket, newMail->subject, sizeof(newMail->subject)-1, 0) == -1){
                perror("RECV SUBJECT error");
            } else {
                printf("Subject: %s: %d\n", newMail->subject, (int)strlen(newMail->subject)); 
            }

            // Receive Message
            do{
                if(recv(*currentClientSocket, newMail->message, sizeof(newMail->message)-1, 0) == -1){
                    perror("RECV Message error");
                } else {
                    printf("Message: %s: %d\n", newMail->message, (int)strlen(newMail->message)); 
                }
            }while(strcmp(newMail->message, ".") != 0);

            free(newMail); 
            printf("MAIL RECEIVED\n");
        } else if(strcmp(buffer, "LIST") == 0){
            printf("LIST COMMAND RECEIVED\n");
            
        } else if(strcmp(buffer, "READ") == 0){
            printf("READ COMMAND RECEIVED\n");
        } else if(strcmp(buffer, "DEL") == 0){
            printf("DEL COMMAND RECEIVED\n");
        } else if(strcmp(buffer, "QUIT") == 0){
            printf("QUIT COMMAND RECEIVED\n");
            break; 
        } else {
            if(send(*currentClientSocket, "ERR", 3, 0) == -1){
                perror("Server failed to send answer"); 
                return NULL; 
            }
            continue; 
        }

        if(send(*currentClientSocket, "OK", 3, 0) == -1){
            perror("Server failed to send answer"); 
            return NULL; 
        }
        

    }while(strcmp(buffer, "QUIT") != 0 && !abortRequested); 

    if(*currentClientSocket != 1){
        if(shutdown(*currentClientSocket, SHUT_RDWR) == -1)
            perror("SHUTDOWN error: currentClientSocket");
        if(close(*currentClientSocket) == -1){
            perror("CLOSE error: currentClientSocket");
        }
        *currentClientSocket = -1;
    }

    return NULL; 
}

void printUsage(){
    fprintf(stdout, "./tw-server <port> <mail-spool-directory>\n"); 
}

void signalHandler(int signal){
    if(signal == SIGINT){
        fprintf(stdout, "Abort Requested..."); 
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