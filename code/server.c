#include "header/serverCommands.h"

void* clientCommunication(void* data); 

int main(int argc, char** argv){
    socklen_t addressLength;
    struct sockaddr_in address, clientAddress; 
    int reuseValue = 1; 

    abortRequested = 0;
    serverSocket = -1;
    newSocket = -1; 

    if(argc != 3){
        printUsage();
        return EXIT_FAILURE; 
    }

    if(argv[2][strlen(argv[2])-1] != '/')
        strcat(argv[2], "/"); 
    sprintf(mail_spool, "mails/%s", argv[2]); 
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
        
        args_t* args = (args_t*)malloc(sizeof(args_t));
        args->ip = clientIP; 
        args->socket = newSocket; 

        // Start Thread
        pthread_create(&clients[clientCount],   // Thread object
                       NULL,                    // Default thread attributes
                       clientCommunication,     // Function
                       (void*)args);            // Function parameters (socket descriptor)
        
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

    args_t* args = (args_t*)data; 
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
