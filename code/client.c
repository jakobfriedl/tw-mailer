#include "header/clientCommands.h"

int main(int argc, char** argv){

    u_int16_t port; 
    int clientSocket = -1; 
    char buffer[BUFFER]; 
    struct sockaddr_in address; 
    int size; 

    int isLoggedIn = 0; 

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
        printf("> "); 
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
            
            if(size == 0) continue;  
            
            // Send Command 
            if((send(clientSocket, buffer, size, 0) == -1)){
                perror("SEND error");
                break; 
            } 

            // Check Command
            if(!strcmp(buffer, "QUIT")){
                isLoggedIn = 0; 
                break;
            }

            if(!isLoggedIn){

                if(!strcmp(buffer, "LOGIN")){

                    sendLoginRequest(clientSocket);
                    if(receiveFeedback(clientSocket)){
                        printf("Login successful, you can use to following commands:\n \
                                \n   SEND \
                                \n   LIST \
                                \n   READ \
                                \n   DEL \
                                \n   QUIT\n\n");
                        isLoggedIn = 1; 
                    } 

                } else {
                    receiveFeedback(clientSocket); 
                }

            }else{

                if(!strcmp(buffer, "SEND")){ 

                    sendSendRequest(clientSocket);
                    receiveFeedback(clientSocket);

                } else if(!strcmp(buffer, "LIST")){
                    
                    sendListRequest(clientSocket);

                } else if(!strcmp(buffer, "READ")){ 

                    sendReadRequest(clientSocket);

                } else if(!strcmp(buffer, "DEL")){

                    sendDelRequest(clientSocket);
                    receiveFeedback(clientSocket); 

                } else {
                    receiveFeedback(clientSocket); 
                }
            }

        }

    }while(strcmp(buffer, "QUIT")); 

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