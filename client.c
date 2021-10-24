#include "header.h"

void printUsage();


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
            quit = strcmp(buffer, "QUIT") == 0; 
            
            if(size == 0) continue; 
            
            // Send Command
            if((send(clientSocket, buffer, size, 0) == -1)){
                perror("SEND error");
                break; 
            } 

            if(strcmp(buffer, "SEND") == 0){
                // Send Mail

                mail_t* newMail = (mail_t*)malloc(sizeof(mail_t));

                // Send Sender
                fgets(newMail->sender, sizeof(newMail->sender), stdin); 
                int senderSize = (int)strlen(newMail->sender); 
                if(newMail->sender[senderSize-2] == '\r' && newMail->sender[senderSize-1] == '\n'){
                    newMail->sender[senderSize] = 0; 
                    senderSize -= 2; 
                }else if(newMail->sender[senderSize-1] == '\n'){
                    newMail->sender[senderSize] = 0; 
                    --senderSize;
                }
                newMail->sender[senderSize] = '\0'; 

                if(writen(clientSocket, newMail->sender, sizeof(newMail->sender)-1) == -1){
                    perror("SEND MAIL CONTENTS error"); 
                } else {
                    printf("sent %s\n", newMail->sender);
                }

                // Send Receiver
                fgets(newMail->receiver, sizeof(newMail->receiver), stdin); 
                int receiverSize = (int)strlen(newMail->receiver); 
                if(newMail->receiver[receiverSize-2] == '\r' && newMail->receiver[receiverSize-1] == '\n'){
                    newMail->receiver[receiverSize] = 0; 
                    receiverSize -= 2; 
                }else if(newMail->receiver[receiverSize-1] == '\n'){
                    newMail->receiver[receiverSize] = 0; 
                    --receiverSize;
                }
                newMail->receiver[receiverSize] = '\0'; 

                if(writen(clientSocket, newMail->receiver, sizeof(newMail->receiver)-1) == -1){
                    perror("SEND MAIL CONTENTS error"); 
                } else {
                    printf("sent %s\n", newMail->receiver);
                }

                // Send Subject
                fgets(newMail->subject, sizeof(newMail->subject), stdin); 
                int subjectSize = (int)strlen(newMail->subject); 
                if(newMail->subject[subjectSize-2] == '\r' && newMail->subject[subjectSize-1] == '\n'){
                    newMail->subject[subjectSize] = 0; 
                    subjectSize -= 2; 
                }else if(newMail->subject[subjectSize-1] == '\n'){
                    newMail->subject[subjectSize] = 0; 
                    --subjectSize;
                }
                newMail->subject[subjectSize] = '\0'; 

                if(writen(clientSocket, newMail->subject, sizeof(newMail->subject)-1) == -1){
                    perror("SEND MAIL CONTENTS error"); 
                } else {
                    printf("sent %s\n", newMail->subject);
                }



                // char payload[BUFFER]; 
                // while(strcmp(fgets(payload, BUFFER-1, stdin),".\n") != 0){
                //     int size = strlen(payload); 
              
                //     // Remove new-lines at the end
                //     if(payload[size-2] == '\r' && payload[size-1] == '\n'){
                //         size -= 2; 
                //         payload[size] = 0; 
                //     }else if(payload[size-1] == '\n'){
                //         --size;
                //         payload[size] = 0; 
                //     }
                //     payload[size] = '\0'; // Terminate String

                //     if(writen(clientSocket, payload, BUFFER-1) == -1){
                //         perror("SEND MAIL CONTENTS error"); 
                //     }   
                // }
                
                free(newMail); 

                printf("MAIL SENT\n");
            } else if(strcmp(buffer, "LIST") == 0){
                printf("LIST COMMAND SENT\n");
            } else if(strcmp(buffer, "READ") == 0){
                printf("READ COMMAND SENT\n");
            } else if(strcmp(buffer, "DEL") == 0){
                printf("DEL COMMAND SENT\n");
            } else if(strcmp(buffer, "QUIT") == 0){
                printf("QUIT COMMAND SENT\n");
            } else {
                printf("Unknown command\n"); 
            }

            // Receive Feedback
            size = recv(clientSocket, buffer, BUFFER-1, 0); 
            if(size == -1)
                perror("RECV error"); 
            else if (size == 0)
                fprintf(stdout, "Server closed remote socket\n"); 
            else{
                buffer[size] = '\0'; // Terminate String
                fprintf(stdout, "%s\n", buffer); 
            }
        }
    }while(!quit); 

    // Close Socket
    if(clientSocket != -1){
        if(shutdown(clientSocket, SHUT_RDWR) == -1){
            perror("SHUTDOWN error: clientSocket");
        }
        if(close(clientSocket) == -1){
            printf("close clientSocket"); 
            perror("CLOSE error: clientSocket");
        }
        clientSocket = -1; 
    }

    return EXIT_SUCCESS; 
}

void printUsage(){
    fprintf(stdout, "./tw-client <ip> <port>\n"); 
}