#include "header.h"
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <ldap.h>
#include <time.h>

// Global Variables
struct stat st = {0}; 

char mail_spool[PATH_MAX]; 

int serverSocket = -1; 
int newSocket = -1; 
int abortRequested = 0; 

// Threading
#define NUM_CLIENTS 100
pthread_mutex_t mutex;
pthread_t clients[NUM_CLIENTS];

typedef struct args{
    int socket; 
    char* ip; 
}t_arg_struct;

// Functions
void printUsage(); 
void signalHandler(int signal); 
void* clientCommunication(void* data); 
void sendFeedback(int socket, char* feedback); 

int handleLoginRequest(int socket, char* sessionUser, char* ip); 
int handleSendRequest(int socket, char* sessionUser); 
int handleListRequest(int socket, char* sessionUser); 
int handleReadRequest(int socket, char* sessionUser); 
int handleDelRequest(int socket, char* sessionUser); 

int main(int argc, char** argv){
    socklen_t addressLength;
    struct sockaddr_in address, clientAddress; 
    int reuseValue = 1; 

    if(argc != 3){
        printUsage();
        return EXIT_FAILURE; 
    }

    if(argv[2][strlen(argv[2])-1] != '/')
        strcat(argv[2], "/"); 
    sprintf(mail_spool, "mail-spool/%s", argv[2]); 
    if(stat(mail_spool, &st) == -1){
        mkdir(mail_spool, 0777); 
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
        char* clientIP = inet_ntoa(clientAddress.sin_addr); 
        fprintf(stdout, "Client connected from %s:%d...\n", clientIP, ntohs(clientAddress.sin_port));
        
        if(pthread_mutex_init(&mutex, NULL)){
            perror("ERR: mutex_init"); 
            exit(EXIT_FAILURE); 
        }
        
        t_arg_struct* args = (t_arg_struct*)malloc(sizeof(t_arg_struct));
        args->ip = clientIP; 
        args->socket = newSocket; 

        // Start Thread
        pthread_create(&clients[clientCount],   // Thread object
                       NULL,                    // Default thread attributes
                       clientCommunication,     // Function
                       (void*)args);           // Function parameters (socket descriptor)
        
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

    t_arg_struct* args = (t_arg_struct*)data; 
    int currentClientSocket = args->socket;
    char* currentClientIP = args->ip; 

    int isLoggedIn = 0; 
    char* sessionUser = (char*)malloc(BUFFER); 

    // Welcome Client
    strcpy(buffer, "\nWelcome to TW-Mail-Server, you can use to following commands:\n \
                    \n   LOGIN \
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

        // Check Command
        if(!strcmp(buffer, "QUIT")){
            printf("QUIT COMMAND RECEIVED\n");
            isLoggedIn = 0; 
            break;
        }

        if(!isLoggedIn){

            if(!strcmp(buffer, "LOGIN")){

                printf("LOGIN COMMAND RECEIVED\n");
                int rc = 0; 
                if((rc = handleLoginRequest(currentClientSocket, sessionUser, currentClientIP)) == -1){
                    sendFeedback(currentClientSocket, "ERR"); 
                }else if (rc == -2){
                    sendFeedback(currentClientSocket, "[ ! ] invalid credentials."); 
                }else if (rc == -3){
                    sendFeedback(currentClientSocket, "[ ! ] too many tries. your ip is locked."); 
                }else{
                    sendFeedback(currentClientSocket, "OK"); 
                    isLoggedIn = 1; 
                }

            } else {
                // Unknown command
                sendFeedback(currentClientSocket, "ERR"); 
            }

        }else{ // Authenticated User

            if(!strcmp(buffer, "SEND")){   

                printf("SEND COMMAND RECEIVED\n");
                if(handleSendRequest(currentClientSocket, sessionUser) == -1){
                    sendFeedback(currentClientSocket, "ERR"); 
                }else{
                    sendFeedback(currentClientSocket, "OK"); 
                }

            } else if(!strcmp(buffer, "LIST")){

                printf("LIST COMMAND RECEIVED\n");
                if(handleListRequest(currentClientSocket, sessionUser) == -1){
                    sendFeedback(currentClientSocket, "ERR"); 
                }

            } else if(!strcmp(buffer, "READ")){

                printf("READ COMMAND RECEIVED\n");
                if(handleReadRequest(currentClientSocket, sessionUser) == -1){
                    sendFeedback(currentClientSocket, "ERR"); 
                }

            } else if(!strcmp(buffer, "DEL")){

                printf("DEL COMMAND RECEIVED\n");
                if(handleDelRequest(currentClientSocket, sessionUser) == -1){
                    sendFeedback(currentClientSocket, "ERR"); 
                }else{
                    sendFeedback(currentClientSocket, "OK"); 
                }
 
            } else {
                // Unknown command
                sendFeedback(currentClientSocket, "ERR"); 
            }

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

    free(sessionUser); 
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
    if(send(socket, feedback, strlen(feedback), 0) == -1){
        perror("Server failed to send answer"); 
        return; 
    }
}

///////////////////////////////////////////
//! LOGIN - FUNCTIONALITY 
///////////////////////////////////////////
int handleLoginRequest(int socket, char* sessionUser, char* ip){
    const char* ldapUri = "ldap://ldap.technikum-wien.at:389";
    const int ldapVersion = LDAP_VERSION3; 
    char* ldapUser = (char*)malloc(BUFFER); 

    char* user = (char*)malloc(BUFFER); 
    char* password = (char*)malloc(BUFFER); 
    int size = 0; 

    // Receive User
    if((size = readline(socket, user, BUFFER)) == -1){
        perror("RECV USER error");
        return -1; 
    }
    user[size] = '\0'; 
    if(!validateUserName(user)){
        return -1; 
    } 
    printf("Username: %s: %d\n", user, (int)strlen(user)); 
    sprintf(ldapUser, "uid=%s,ou=people,dc=technikum-wien,dc=at", user); 

    // Receive Password
    if((size = readline(socket, password, BUFFER)) == -1){
        perror("RECV PASSWORD error");
        return -1; 
    }
    password[size] = '\0'; 

    // Ldap Connection
    int rc = 0; 
    LDAP* ldapHandle; 
    if((rc = ldap_initialize(&ldapHandle, ldapUri)) != LDAP_SUCCESS){
        fprintf(stderr, "ldap_init failed\n"); 
        return -1; 
    }

    // Set Version Options
    if((rc = ldap_set_option(ldapHandle, LDAP_OPT_PROTOCOL_VERSION, &ldapVersion)) != LDAP_OPT_SUCCESS){
        fprintf(stderr, "ldap_set_option(PROTOCOL_VERSION): %s\n", ldap_err2string(rc)); 
        return -1; 
    }

    // Start TSL Connection
    if((rc = ldap_start_tls_s(ldapHandle, NULL, NULL)) != LDAP_SUCCESS){
        fprintf(stderr, "ldap_start_tls_s(): %s\n", ldap_err2string(rc));
        return -1; 
    }

    // Variables for Blacklist
    char* ipBlacklist = (char*)malloc(PATH_MAX);
    char* ipBlacklistCounter = (char*)malloc(PATH_MAX); 
    char* ipBlacklistTimestamp = (char*)malloc(PATH_MAX); 
    sprintf(ipBlacklist, "blacklist/%s", ip); 
    sprintf(ipBlacklistCounter, "blacklist/%s/counter", ip); 
    sprintf(ipBlacklistTimestamp, "blacklist/%s/timestamp", ip); 

    // Create Blacklist Directory for User
    if(stat(ipBlacklist, &st) == -1){
        mkdir(ipBlacklist, 0777);

        // Initialize Counter with 0
        FILE* counterFile = fopen(ipBlacklistCounter, "w"); 
        fputs("0", counterFile); 
        fclose(counterFile); 
    }

    FILE* checkTimestamp = fopen(ipBlacklistTimestamp, "r"); 
    if(checkTimestamp){
        char* timestamp = (char*)malloc(BUFFER); 
        fgets(timestamp, BUFFER, checkTimestamp); 

        if((strcmp(timestamp, " ")) && (time(0) - atoi(timestamp)< 60)){
            // User is Locked
            free(timestamp); 
            free(ipBlacklist);
            free(ipBlacklistCounter);
            free(ipBlacklistTimestamp);
            return -3; 
        }
        free(timestamp); 
    }

    // Bind Credentials
    BerValue bindCredentials; 
    bindCredentials.bv_val = password; 
    bindCredentials.bv_len = strlen(password); 
    BerValue *serverCredentials; 

    if((rc = ldap_sasl_bind_s(
        ldapHandle,
        ldapUser,
        LDAP_SASL_SIMPLE,
        &bindCredentials,
        NULL,
        NULL,
        &serverCredentials
    )) != LDAP_SUCCESS){

        // Invalid Credentials
        FILE* readCounter = fopen(ipBlacklistCounter, "r");

        char* currentCounter = (char*)malloc(BUFFER); 
        char* newCounter = (char*)malloc(BUFFER); 

        fgets(currentCounter, BUFFER,  readCounter); 
        int currentCounterInt = atoi(currentCounter); 
        currentCounterInt++; 
        if(currentCounterInt < 3){
            sprintf(newCounter, "%d", currentCounterInt); 
        }else{

            // Save current timestamp to timestamp file
            FILE* timestamp = fopen(ipBlacklistTimestamp, "w"); 
            char* currentTime = (char*)malloc(BUFFER);
            sprintf(currentTime, "%lu", (unsigned long)time(0)); 
            fputs(currentTime, timestamp); 
            fclose(timestamp); 
            strcpy(newCounter, "0"); 

            free(currentTime); 

            return -3; 
        }
        fclose(readCounter);  

        FILE* updateCounter = fopen(ipBlacklistCounter, "w");
        fputs(newCounter, updateCounter); 
        fclose(updateCounter); 

        fprintf(stderr, "LDAP bind error: %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ldapHandle, NULL, NULL);

        free(ipBlacklist);
        free(ipBlacklistCounter);
        free(ipBlacklistTimestamp);
        free(newCounter);
        free(currentCounter); 
        return -2; 
    }
        
    // Login successful -> Reset Counter
    printf("Login successful\n"); 
    FILE* resetCounter = fopen(ipBlacklistCounter, "w"); 
    fputs("0", resetCounter);
    fclose(resetCounter); 

    // Set Session
    strcpy(sessionUser, user);
    sessionUser[strlen(sessionUser)] = '\0';  

    free(ldapUser);
    free(user);
    free(password); 

    free(ipBlacklist);
    free(ipBlacklistCounter);
    free(ipBlacklistTimestamp);

    ldap_unbind_ext_s(ldapHandle, NULL, NULL);

    return 0; 
}

///////////////////////////////////////////
//! SEND - FUNCTIONALITY 
///////////////////////////////////////////
int handleSendRequest(int socket, char* sessionUser){
    mail_t* newMail = (mail_t*)malloc(sizeof(mail_t));
    int size = 0; 
 
    // Receive Sender from Session
    strcpy(newMail->sender, sessionUser); 
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

    if(pthread_mutex_lock(&mutex)){
        perror("ERR: mutex_lock"); 
        return -1;  
    }
    // Create directory and Index File
    if(stat(directory, &st) == -1){
        mkdir(directory, 0777);
        sprintf(indexFile, "%s/index", directory);  
        FILE* index = fopen(indexFile, "w"); 
        fputs("1", index); 
        fclose(index); 
    }
    pthread_mutex_unlock(&mutex); 

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

    if(pthread_mutex_lock(&mutex)){
        perror("ERR: mutex_lock"); 
        return -1;  
    }
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
    
    pthread_mutex_unlock(&mutex); 

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
        if(!size){ 
            // Client aborted with CTRL+C
            fputs(".\n", mailFile); // Put .\n to make file readable
            break; 
        }
        newMail->message[size] = '\0'; 
        printf("Message: %s: %d\n", newMail->message, (int)strlen(newMail->message)); 
        fputs(strcat(newMail->message, "\n"), mailFile); 
    }while(strcmp(newMail->message, ".\n") && !abortRequested);

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
int handleListRequest(int socket, char* sessionUser){
    printf("Username: %s: %d\n", sessionUser, (int)strlen(sessionUser)); 
    
    char* directory = (char*)malloc(PATH_MAX);
    char* pathToFile = (char*)malloc(PATH_MAX);
    sprintf(directory, "%s%s", mail_spool, sessionUser); 

    DIR *dir = opendir(directory); 
    struct dirent *dirEntry; 
    int mailCount = 0; 
    char mails[BUFFER][BUFFER]; 

    if(!dir){
        // User does not exist -> only send mailcount (0)
        int convertedMailCount = htonl(mailCount); // Convert from host to network 
        if(writen(socket, &convertedMailCount, sizeof(convertedMailCount)) == -1){
            perror("SEND error"); 
        }
        return 0; 
    }
    while((dirEntry = readdir(dir))){
        if(strcmp(dirEntry->d_name, ".") && strcmp(dirEntry->d_name, "..")){ 
            sprintf(pathToFile, "%s/%s", directory, dirEntry->d_name);

            if(pthread_mutex_lock(&mutex)){
                perror("ERR: mutex_lock"); 
                return -1;  
            }
            // Open file in "read"-mode
            FILE* file = fopen(pathToFile, "r"); 
            pthread_mutex_unlock(&mutex); 
            
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
            // printf("done in thread %d\n", (int)pthread_self());
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

    free(directory); 
    free(pathToFile); 
    return 0; 
}

///////////////////////////////////////////
//! READ - FUNCTIONALITY
///////////////////////////////////////////
int handleReadRequest(int socket, char* sessionUser){
    int size = 0; 
    char* mailNumber = (char*)malloc(BUFFER * sizeof(char)); 

    printf("Username: %s: %d\n", sessionUser, (int)strlen(sessionUser)); 

    // Receive Mail-Number
    if((size = readline(socket, mailNumber, BUFFER)) == -1){
        perror("RECV SENDER error");
        return -1; 
    }
    mailNumber[size] = '\0';
    printf("MailNr.: %s: %d\n", mailNumber, (int)strlen(mailNumber));

    char* directory = (char*)malloc(PATH_MAX);
    char* pathToFile = (char*)malloc(PATH_MAX);

    sprintf(pathToFile, "%s%s/%s", mail_spool, sessionUser, mailNumber);

    if(pthread_mutex_lock(&mutex)){
        perror("ERR: mutex_lock"); 
        return -1;  
    } 
    FILE* mail = fopen(pathToFile, "r"); 
    pthread_mutex_unlock(&mutex); 
    
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
    free(mailNumber); 

    return 0;
}

///////////////////////////////////////////
//! DEL - FUNCTIONALITY
///////////////////////////////////////////
int handleDelRequest(int socket, char* sessionUser){
    int size = 0;
    char* mailNumber = (char*)malloc(BUFFER * sizeof(char)); 

    printf("Username: %s: %d\n", sessionUser, (int)strlen(sessionUser)); 

    // Receive Mail-Number
    if((size = readline(socket, mailNumber, BUFFER)) == -1){
        perror("RECV SENDER error");
        return -1; 
    }
    mailNumber[size] = '\0';
    printf("MailNr.: %s: %d\n", mailNumber, (int)strlen(mailNumber));

    char* directory = (char*)malloc(PATH_MAX);
    char* pathToFile = (char*)malloc(PATH_MAX);
    
    sprintf(pathToFile, "%s%s/%s", mail_spool, sessionUser, mailNumber); 
    if(pthread_mutex_lock(&mutex)){
        perror("ERR: mutex_lock"); 
        return -1;  
    }
    int isFound = remove(pathToFile); // On-error, returns -1
    pthread_mutex_unlock(&mutex); 

    free(directory);
    free(pathToFile); 
    free(mailNumber); 

    return isFound; 
}