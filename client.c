#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

#define BUFFER 1024

void PrintUsage(); 

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
        PrintUsage(); 
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
            // Remote new-lines at the end
            if(buffer[size-2] == '\r' && buffer[size-1] == '\n'){
                size -= 2; 
                buffer[size] = 0; 
            }else if(buffer[size-1] == '\n'){
                --size;
                buffer[size] = 0; 
            }
            quit = strcmp(buffer, "QUIT") == 0; 

            // Send Data
            if((send(clientSocket, buffer, size, 0) == -1)){
                perror("SEND error");
                break; 
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

void PrintUsage(){
    fprintf(stdout, "./tw-client <ip> <port>\n"); 
}