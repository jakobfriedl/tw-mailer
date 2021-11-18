#include "header.h"
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>

struct stat st = {0}; 

char mail_spool[PATH_MAX]; 

int serverSocket = -1; 
int newSocket = -1; 
int abortRequested = 0; 

// Definitions for Threading
#define NUM_CLIENTS 100
pthread_mutex_t mutex;
pthread_t clients[NUM_CLIENTS];
pthread_attr_t attributes[NUM_CLIENTS];

// Functions
void printUsage(); 
void signalHandler(int signal); 
void* clientCommunication(void* data); 
void sendFeedback(int socket, char* feedback); 

int handleLoginRequest(int socket); 
int handleSendRequest(int socket); 
int handleListRequest(int socket); 
int handleReadRequest(int socket); 
int handleDelRequest(int socket); 

int main(int argc, char** argv){
    socklen_t addressLength;
    struct sockaddr_in address, clientAddress; 
    int reuseValue = 1; 

    if(argc != 3){
        printUsage();
        return EXIT_FAILURE; 
    }

    strcpy(mail_spool, argv[2]); 
    if(mail_spool[strlen(mail_spool)-1] != '/')
        strcat(mail_spool, "/"); 

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

    int clientCount = 0; 
    while(!abortRequested){
        fprintf(stdout, "Waiting for connection...\n"); 

        // Accept Connection Setup
        addressLength = sizeof(struct sockaddr_in); 
        if((newSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &addressLength)) == -1){
            if(!abortRequested){
                perror("ACCEPT error"); 
            } 
            break; 
        }
        // Start Client
        fprintf(stdout, "Client connected from %s:%d...\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
        
        pthread_mutex_init(&mutex, NULL); 
        pthread_attr_init(&attributes[clientCount]); 
        int socket = newSocket;

        // Start Thread
        pthread_create(&clients[clientCount],           // Thread object
                       &attributes[clientCount],        // Default thread attributes
                       clientCommunication,             // Function
                       (void*)&socket);                 // Function parameters (socket descriptor, client number)
        
        clientCount++; 

        newSocket = -1; 
    }

    // Wait for all threads to finish
    for(int i = 0; i < clientCount; i++){
        printf("Waiting for client %d to finish...\n", i); 
        if(pthread_join(clients[i], NULL)){
            fprintf(stderr, "ERR: join not successful");
        }else{
            printf("Client %d is done.\n", i); 
        }
    }
    printf("All clients are done.\n"); 

    // Destroy mutex
    pthread_mutex_destroy(&mutex); 

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
    int currentClientSocket = *(int*)data; 

    // Welcome Client
    strcpy(buffer, "\nWelcome to TW-Mail-Server, you can use to following commands:\n \
                    \n   LOGIN \
                    \n   SEND \
                    \n   LIST \
                    \n   READ \
                    \n   DEL \
                    \n   QUIT\n");
    if (send(currentClientSocket, buffer, strlen(buffer), 0) == -1){
        perror("SEND error");
        return NULL;
    }

    do{ 
        // Receive Command
        size = recv(currentClientSocket, buffer, sizeof(buffer)-1, 0);
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

        if(!strcmp(buffer, "LOGIN")){

            printf("LOGIN COMMAND RECEIVED\n");

        } else if(!strcmp(buffer, "SEND")){   

            printf("SEND COMMAND RECEIVED\n");
            if(handleSendRequest(currentClientSocket) == -1){
                sendFeedback(currentClientSocket, "ERR"); 
            }else{
                sendFeedback(currentClientSocket, "OK"); 
            }

        } else if(!strcmp(buffer, "LIST")){

            printf("LIST COMMAND RECEIVED\n");
            if(handleListRequest(currentClientSocket) == -1){
                sendFeedback(currentClientSocket, "ERR"); 
            }

        } else if(!strcmp(buffer, "READ")){

            printf("READ COMMAND RECEIVED\n");
            if(handleReadRequest(currentClientSocket) == -1){
                sendFeedback(currentClientSocket, "ERR"); 
            }

        } else if(!strcmp(buffer, "DEL")){

            printf("DEL COMMAND RECEIVED\n");
            if(handleDelRequest(currentClientSocket) == -1){
                sendFeedback(currentClientSocket, "ERR"); 
            }else{
                sendFeedback(currentClientSocket, "OK"); 
            }

        } else if(!strcmp(buffer, "QUIT")){
            printf("QUIT COMMAND RECEIVED\n");
            break; 
        } else {
            // Unknown command
            sendFeedback(currentClientSocket, "ERR"); 
        }

    }while(strcmp(buffer, "QUIT") && !abortRequested); 

    if(currentClientSocket != 1){
        if(shutdown(currentClientSocket, SHUT_RDWR) == -1)
            perror("SHUTDOWN error:currentClientSocket");
        if(close(currentClientSocket) == -1){
            perror("CLOSE error:currentClientSocket");
        }
        currentClientSocket = -1;
    }

    pthread_exit(NULL);
    return NULL; 
}

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
    if(send(socket, feedback, sizeof(feedback), 0) == -1){
        perror("Server failed to send answer"); 
        return; 
    }
}

///////////////////////////////////////////
//! LOGIN - FUNCTIONALITY 
///////////////////////////////////////////
int handleLoginRequest(int socket){
    return 0; 
}

///////////////////////////////////////////
//! SEND - FUNCTIONALITY 
///////////////////////////////////////////
int handleSendRequest(int socket){

    mail_t* newMail = (mail_t*)malloc(sizeof(mail_t));
    int size = 0; 
 
    // Receive Sender
    if((size = readline(socket, newMail->sender, sizeof(newMail->sender))) == -1){
        perror("RECV SENDER error");
        return -1; 
    }
    newMail->sender[size] = '\0'; 
    if(!validateUserName(newMail->sender)){
        return -1; 
    } 
    printf("Sender: %s: %d\n", newMail->sender, (int)strlen(newMail->sender)); 

    // Receive Receiver
    if((size = readline(socket, newMail->receiver, sizeof(newMail->receiver))) == -1){
        perror("RECV RECEIVER error");
        return -1; 
    }
    newMail->receiver[size] = '\0';  
    if(!validateUserName(newMail->receiver)){
        return -1; 
    }
    printf("Receiver: %s: %d\n", newMail->receiver, (int)strlen(newMail->receiver)); 

    // Create Directory for receiver if it doesnt already exist
    char* directory = (char*)malloc(PATH_MAX);
    sprintf(directory, "%s%s", mail_spool, newMail->receiver);

    char* indexFile = (char*)malloc(PATH_MAX); 

    // Create directory and Index File
    if(stat(directory, &st) == -1){
        mkdir(directory, 0777);
        sprintf(indexFile, "%s/index", directory);  
        FILE* index = fopen(indexFile, "w"); 
        fputs("1", index); 
        fclose(index); 
    }

    // Receive Subject
    if((size = readline(socket, newMail->subject, sizeof(newMail->subject))) == -1){
        perror("RECV SUBJECT error");
        return -1; 
    }
    newMail->subject[size] = '\0'; 
    if(strlen(newMail->subject) > SUBJECT_LENGTH){
        return -1; 
    }
    printf("Subject: %s: %d\n", newMail->subject, (int)strlen(newMail->subject)); 
    
    char* fileName = (char*)malloc(PATH_MAX);; 
    FILE* mailFile = NULL; 
    char* completeFileName = (char*)malloc(PATH_MAX); 

    // Find out the correct Mail-Number to access the mail later
    sprintf(indexFile, "%s/index", directory);  
    FILE* index = fopen(indexFile, "r"); 
    char highestNumber[BUFFER]; 
    fgets(highestNumber, sizeof(highestNumber), index); 
    fclose(index);

    // Construct File Path: mail-spool/username/number
    sprintf(completeFileName, "%s/%s", directory, highestNumber);
    
    // Update Index File with next highest Number
    char nextHighest[BUFFER];
    sprintf(nextHighest, "%d", atoi(highestNumber)+1); 
    FILE* updatedIndex = fopen(indexFile, "w"); 
    fputs(nextHighest, updatedIndex);
    fclose(updatedIndex); 

    // Open file in "append"-mode   
    mailFile = fopen(completeFileName, "a"); 

    if(mailFile == NULL){
        return -1; 
    }

    fputs(strcat(newMail->sender, "\n"), mailFile); 
    fputs(strcat(newMail->receiver, "\n"), mailFile);
    fputs(strcat(newMail->subject, "\n"), mailFile); 

    // Receive Message
    do{
        if((size = readline(socket, newMail->message, sizeof(newMail->message))) == -1){
            perror("RECV Message error");
            return -1; 
        }
        newMail->message[size] = '\0'; 
        printf("Message: %s: %d\n", newMail->message, (int)strlen(newMail->message)); 
        fputs(strcat(newMail->message, "\n"), mailFile); 
    }while(strcmp(newMail->message, ".\n"));

    fclose(mailFile); 

    free(directory);
    free(fileName); 
    free(completeFileName); 
    free(newMail); 
    printf("MAIL RECEIVED\n");

    return 0; 
}

///////////////////////////////////////////
//! LIST - FUNCTIONALITY
///////////////////////////////////////////
int handleListRequest(int socket){
    char* user = (char*)malloc(BUFFER * sizeof(char)); 
    int size = 0; 

    // Receive Username
    if((size = readline(socket, user, BUFFER)) == -1){
        perror("RECV SENDER error");
        return -1; 
    }
    user[size] = '\0'; 
    if(!validateUserName(user)){
        return -1; 
    }
    printf("Username: %s: %d\n", user, (int)strlen(user)); 
    
    char* directory = (char*)malloc(PATH_MAX);
    char* pathToFile = (char*)malloc(PATH_MAX);
    sprintf(directory, "%s%s", mail_spool, user); 

    DIR *dir = opendir(directory); 
    struct dirent *dirEntry; 
    int mailCount = 0; 
    char mails[BUFFER][BUFFER]; 

    if(!dir){
        // User does not exist
        int convertedMailCount = htonl(mailCount); // Convert from host to network 
        if(writen(socket, &convertedMailCount, sizeof(convertedMailCount)) == -1){
            perror("SEND error"); 
        }
        return -1; 
    }
    while((dirEntry = readdir(dir))){
        if(strcmp(dirEntry->d_name, ".") && strcmp(dirEntry->d_name, "..")){ 
            sprintf(pathToFile, "%s/%s", directory, dirEntry->d_name);

            // Open file in "read"-mode
            FILE* file = fopen(pathToFile, "r"); 
            int count = 0;   
            
            if(file != NULL){
                char line[BUFFER]; 
                while(fgets(line, sizeof(line), file) != NULL){
                    if(count == 2){ // Get subject from mail
                        line[strlen(line)-1] = '\0'; // Remove \n and replace it with \0    
                        // Construct Output string shown to client, contains mail-number and subject                        
                        sprintf(mails[mailCount], "%s - %s", dirEntry->d_name, line); 
                        mailCount++;  
                    }
                    count++; 
                }
                fclose(file); 
            }
        }
    }
    closedir(dir); 
    free(dirEntry);

    int convertedMailCount = htonl(mailCount); // Convert from host to network 
    if(writen(socket, &convertedMailCount, sizeof(convertedMailCount)) == -1){
        perror("SEND error"); 
    }
    
    for(int i = 0; i < mailCount; i++){
        if(writen(socket, mails[i], BUFFER) == -1){
            perror("SEND error"); 
        }
    }

    free(user); 
    free(directory); 
    free(pathToFile); 
    return 0; 
}

///////////////////////////////////////////
//! READ - FUNCTIONALITY
///////////////////////////////////////////
int handleReadRequest(int socket){
    char* user = (char*)malloc(BUFFER * sizeof(char)); 
    char* mailNumber = (char*)malloc(BUFFER * sizeof(char)); 
    int size = 0; 

    // Receive Username
    if((size = readline(socket, user, BUFFER)) == -1){
        perror("RECV SENDER error");
        return -1; 
    }
    user[size] = '\0';
    if(!validateUserName(user)){
        return -1; 
    }
    printf("Username: %s: %d\n", user, (int)strlen(user)); 

    // Receive Mail-Number
    if((size = readline(socket, mailNumber, BUFFER)) == -1){
        perror("RECV SENDER error");
        return -1; 
    }
    mailNumber[size] = '\0';
    printf("MailNr.: %s: %d\n", mailNumber, (int)strlen(mailNumber));

    char* directory = (char*)malloc(PATH_MAX);
    char* pathToFile = (char*)malloc(PATH_MAX);

    sprintf(pathToFile, "%s%s/%s", mail_spool, user, mailNumber); 
    FILE* mail = fopen(pathToFile, "r"); 
    if(!mail){
        return -1; // File or User not found
    }
    sendFeedback(socket, "OK"); 

    char line[BUFFER]; 
    while(fgets(line, sizeof(line), mail) != NULL){
        line[strlen(line)-1] = '\0'; 
        if(writen(socket, line, sizeof(line)) == -1){
            perror("SEND error"); 
            return -1; 
        }
    }
    fclose(mail); 

    free(directory);
    free(pathToFile); 
    free(user);
    free(mailNumber); 

    return 0;
}

///////////////////////////////////////////
//! DEL - FUNCTIONALITY
///////////////////////////////////////////
int handleDelRequest(int socket){
    char* user = (char*)malloc(BUFFER * sizeof(char)); 
    char* mailNumber = (char*)malloc(BUFFER * sizeof(char)); 
    int size = 0;

    // Receive Username
    if((size = readline(socket, user, BUFFER)) == -1){
        perror("RECV SENDER error");
        return -1; 
    }
    user[size] = '\0';
    if(!validateUserName(user)){
        return -1; 
    }
    printf("Username: %s: %d\n", user, (int)strlen(user)); 

    // Receive Mail-Number
    if((size = readline(socket, mailNumber, BUFFER)) == -1){
        perror("RECV SENDER error");
        return -1; 
    }
    mailNumber[size] = '\0';
    printf("MailNr.: %s: %d\n", mailNumber, (int)strlen(mailNumber));

    char* directory = (char*)malloc(PATH_MAX);
    char* pathToFile = (char*)malloc(PATH_MAX);
    
    sprintf(pathToFile, "%s%s/%s", mail_spool, user, mailNumber); 
    int isFound = remove(pathToFile); // On-error, returns -1

    free(directory);
    free(pathToFile); 
    free(user);
    free(mailNumber); 

    return isFound; 
}