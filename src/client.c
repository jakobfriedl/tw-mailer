#include "header.h"

// Functions
void printUsage();
int sendData(int socket, char* buffer, int bytesToSend); 
int receiveFeedback(int socket); 

int sendLoginRequest(int socket);
int sendSendRequest(int socket);
int sendListRequest(int socket);
int sendReadRequest(int socket);
int sendDelRequest(int socket);

int main(int argc, char** argv){

    u_int16_t port; 
    int clientSocket; 
    char buffer[BUFFER]; 
    struct sockaddr_in address; 
    int size; 
    int quit = 0;

    // Create Socket
    if((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("CREATE error"); 
        return EXIT_FAILURE;
    }
    
    // Initialize IP-Address
    memset(&address, 0, sizeof(address)); 
    address.sin_family = AF_INET;
    
    if(argc == 2){
        inet_aton("127.0.0.1", &address.sin_addr);
        port = atoi(argv[1]); 
        address.sin_port = htons(port); 
    }else if (argc == 3){
        inet_aton(argv[1], &address.sin_addr);
        port = atoi(argv[2]); 
        address.sin_port = htons(port); 
    }else{
        printUsage(); 
        return EXIT_FAILURE; 
    }

    // Create Connection
    if(connect(clientSocket, (struct sockaddr *)&address, sizeof(address)) == -1){
        perror("CONNECT error"); 
        return EXIT_FAILURE; 
    }

    fprintf(stdout, "Connection with server %s established on port %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port)); 

    // Receive Data
    size = recv(clientSocket, buffer, BUFFER-1, 0); 
    if(size == -1)
        perror("RECV error"); 
    else if (size == 0)
        fprintf(stdout, "Server closed remote socket\n"); 
    else{
        buffer[size] = '\0'; // Terminate String
        fprintf(stdout, "%s\n", buffer); 
    }

    do{
        if(fgets(buffer, BUFFER, stdin) != NULL){
            int size = strlen(buffer); 

            // Remove new-lines at the end
            if(buffer[size-2] == '\r' && buffer[size-1] == '\n'){
                size -= 2; 
                buffer[size] = 0; 
            }else if(buffer[size-1] == '\n'){
                --size;
                buffer[size] = 0; 
            }
            buffer[size] = '\0'; 
            quit = (!strcmp(buffer, "QUIT"));  
            
            if(size == 0) continue;  
            
            // Send Command 
            if((send(clientSocket, buffer, size, 0) == -1)){
                perror("SEND error");
                break; 
            } 

            if(!strcmp(buffer, "LOGIN")){

                sendLoginRequest(clientSocket);

            }else if(!strcmp(buffer, "SEND")){ 

                if(sendSendRequest(clientSocket) == -1){
                    receiveFeedback(clientSocket); 
                    continue; 
                }
                receiveFeedback(clientSocket);

            } else if(!strcmp(buffer, "LIST")){
                
                if(sendListRequest(clientSocket) == -1){
                    receiveFeedback(clientSocket);
                    continue; 
                }

            } else if(!strcmp(buffer, "READ")){ 

                if(sendReadRequest(clientSocket) == -1){
                    receiveFeedback(clientSocket);
                    continue; 
                }

            } else if(!strcmp(buffer, "DEL")){

                if(sendDelRequest(clientSocket) == -1){
                    receiveFeedback(clientSocket); 
                    continue; 
                } 
                receiveFeedback(clientSocket); 

            } else if(!strcmp(buffer, "QUIT")){
                break; 
            } else {
                receiveFeedback(clientSocket); 
            }
        }
    }while(!quit); 

    // Close Socket
    if(clientSocket != -1){
        if(shutdown(clientSocket, SHUT_RDWR) == -1){
            perror("SHUTDOWN error: clientSocket");
        }
        if(close(clientSocket) == -1){
            perror("CLOSE error: clientSocket");
        } 
        clientSocket = -1; 
    }

    return EXIT_SUCCESS; 
}

void printUsage(){
    fprintf(stdout, "./twmailer-client <ip> <port>\n"); 
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

int receiveFeedback(int socket){
    // Receive Feedback
    char buffer[BUFFER]; 
    int size = recv(socket, buffer, BUFFER-1, 0); 
    if(size == -1)
        perror("RECV error"); 
    else if (size == 0)
        fprintf(stdout, "Server closed remote socket\n"); 
    else{
        buffer[size] = '\0'; // Terminate String
        fprintf(stdout, "%s\n", buffer); 

        if(!strcmp(buffer, "OK"))
            return 1; 
    }
    return 0;
}

///////////////////////////////////////////
//* LOGIN - FUNCTIONALITY 
///////////////////////////////////////////
int sendLoginRequest(int socket){
    return 0; 
}

///////////////////////////////////////////
//* SEND - FUNCTIONALITY 
///////////////////////////////////////////
int sendSendRequest(int socket){
    mail_t* newMail = (mail_t*)malloc(sizeof(mail_t));

    // Send Sender
    if(sendData(socket, newMail->sender, sizeof(newMail->sender)) == -1){
        perror("SEND SENDER error"); 
    } 
    if(!validateUserName(newMail->sender)){ 
        return -1; 
    }
    
    // Send Receiver
    if(sendData(socket, newMail->receiver, sizeof(newMail->receiver)) == -1){
        perror("SEND RECEIVER error"); 
    } 
    if(!validateUserName(newMail->receiver)){
        return -1;  
    }

    // Send Subject
    if(sendData(socket, newMail->subject, sizeof(newMail->subject)) == -1){
        perror("SEND SUBJECT error"); 
    } 
    if(strlen(newMail->subject) > SUBJECT_LENGTH){
        return -1;  
    }

    // Send Message
    do{
        if(sendData(socket, newMail->message, sizeof(newMail->message)) == -1){
            perror("SEND MESSAGE error"); 
        } 
    }while(strcmp(newMail->message, ".") != 0);

    free(newMail); 

    return 0; 
}

///////////////////////////////////////////
//* LIST - FUNCTIONALITY 
///////////////////////////////////////////
int sendListRequest(int socket){
    char* user = (char*)malloc(BUFFER * sizeof(char)); 

    // Send Username
    if(sendData(socket, user, BUFFER) == -1){
        perror("SEND USER error");  
    } 
    if(!validateUserName(user)){ 
        return -1; 
    }
    
    int receivedMailCount; 
    if(recv(socket, &receivedMailCount, sizeof(receivedMailCount), 0) == -1){
        perror("RECV error"); 
    }
    int mailCount = ntohl(receivedMailCount); // Convert from network to host
    char mails[BUFFER][BUFFER]; 

    printf("Number of Messages: %d\n", mailCount); 
    int size = 0; 
    for(int i = 0; i < mailCount; i++){
        if((size = recv(socket, mails[i], BUFFER, 0)) == -1){ 
            perror("RECV error");
        }
        mails[i][size] = '\0'; 
        printf(" %s\n", mails[i]);
    }

    free(user); 

    return 0; 
}

///////////////////////////////////////////
//* READ - FUNCTIONALITY 
///////////////////////////////////////////
int sendReadRequest(int socket){
    char* user = (char*)malloc(BUFFER * sizeof(char)); 
    char* mailNumber = (char*)malloc(BUFFER * sizeof(char)); 

    // Send Username
    if(sendData(socket, user, BUFFER) == -1){
        perror("SEND USER error"); 
    } 
    if(!validateUserName(user)){ 
        return -1; 
    }

    // Send MailNumber
    if(sendData(socket, mailNumber, BUFFER) == -1){
        perror("SEND MAILNR error"); 
    } 

    // Check if Server answers with OK or ERR
    if(receiveFeedback(socket)){
        char line[BUFFER];
        int size = 0; 

        // Receive Mail Contents
        do{
            if((size = recv(socket, line, BUFFER, 0)) == -1){
                perror("RECV error"); 
            }
            line[size] = '\0'; 
            printf("%s\n", line); 
        }while(strcmp(line, "."));
    }

    free(user); 
    free(mailNumber); 

    return 0; 
}

///////////////////////////////////////////
//* DEL - FUNCTIONALITY 
///////////////////////////////////////////
int sendDelRequest(int socket){
    char* user = (char*)malloc(BUFFER * sizeof(char)); 
    char* mailNumber = (char*)malloc(BUFFER * sizeof(char)); 

    // Send Username
    if(sendData(socket, user, BUFFER) == -1){
        perror("SEND USER error"); 
    } 
    if(!validateUserName(user)){ 
        return -1; 
    }

    // Send MailNumber
    if(sendData(socket, mailNumber, BUFFER) == -1){
        perror("SEND MAILNR error"); 
    } 

    free(user); 
    free(mailNumber); 

    return 0; 
}