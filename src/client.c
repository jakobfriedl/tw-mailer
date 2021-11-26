#include "header.h"
#include <termios.h>

// Functions
void printUsage();
int receiveFeedback(int socket); 
void getPassword(char* pw);

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
            if(!isLoggedIn){

                if(!strcmp(buffer, "LOGIN")){

                    sendLoginRequest(clientSocket);
                    if(receiveFeedback(clientSocket)){
                        isLoggedIn = 1; 
                    } 

                } else if(!strcmp(buffer, "QUIT")){
                    isLoggedIn = 0; 
                    break; 
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

                } else if(!strcmp(buffer, "QUIT")){
                    break; 
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

void printUsage(){
    fprintf(stdout, "./twmailer-client <ip> <port>\n"); 
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

// Hides Password when typing it to console
void getPassword(char* pw){
    static struct termios oldt, newt;
    int c, i = 0;

    // Saving the old settings of STDIN_FILENO and copy settings for resetting
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    
    // Setting the approriate bit in the termios struct
    newt.c_lflag &= ~(ECHO);          

    /// Setting the new bits
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // Reading the password from the console
    while ((c = getchar())!= '\n' && c != EOF && i < BUFFER){
        pw[i++] = c;
    }
    pw[i] = '\0';

    // Resetting our old STDIN_FILENO
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n"); 
}

///////////////////////////////////////////
//* LOGIN - FUNCTIONALITY 
///////////////////////////////////////////
int sendLoginRequest(int socket){
    char* user = (char*)malloc(BUFFER); 
    char* password = (char*)malloc(BUFFER); 

    // Send Username
    do{
        printf("username: "); 
        fgets(user, BUFFER, stdin); 
        user[strlen(user)-1] = '\0'; 
    } while(!validateUserName(user));

    if(sendData(socket, user, BUFFER) == -1){
        perror("SEND USER error"); 
    } 

    // Send Passwort
    printf("password: "); 
    getPassword(password); // Hide on Input
    if(sendData(socket, password, BUFFER) == -1){
        perror("SEND PASSWORD error"); 
    } 

    return 0;
}

///////////////////////////////////////////
//* SEND - FUNCTIONALITY 
///////////////////////////////////////////
int sendSendRequest(int socket){
    mail_t* newMail = (mail_t*)malloc(sizeof(mail_t));
    
    // Send Receiver
    do {
        printf("to: "); 
        fgets(newMail->receiver, sizeof(newMail->receiver), stdin); 
        newMail->receiver[strlen(newMail->receiver)-1] = '\0'; 
    } while(!validateUserName(newMail->receiver));

    if(sendData(socket, newMail->receiver, sizeof(newMail->receiver)) == -1){
        perror("SEND RECEIVER error"); 
    } 

    // Send Subject
    int c = 0; 
    do {
        if(c > 0) printf("[ ! ] subject too long.\n");
        printf("subject: "); 
        fgets(newMail->subject, sizeof(newMail->subject), stdin);
        newMail->subject[strlen(newMail->subject)-1] = '\0'; 
        c++; 
    } while(strlen(newMail->subject) > SUBJECT_LENGTH || strlen(newMail->subject) <= 0);
    
    if(sendData(socket, newMail->subject, sizeof(newMail->subject)) == -1){
        perror("SEND SUBJECT error"); 
    } 

    // Send Message
    printf("-------------\n"); 
    do{
        fgets(newMail->message, sizeof(newMail->message), stdin); 
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
    
    int mailCount = 0;
    int receivedMailCount = 0; 
    if(recv(socket, &receivedMailCount, sizeof(receivedMailCount), 0) == -1){
        perror("RECV error"); 
    }
    mailCount = ntohl(receivedMailCount); // Convert from network to host
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

    return 0; 
}

///////////////////////////////////////////
//* READ - FUNCTIONALITY 
///////////////////////////////////////////
int sendReadRequest(int socket){
    char* mailNumber = (char*)malloc(BUFFER); 

    // Send MailNumber
    printf("id: "); 
    fgets(mailNumber, BUFFER, stdin); 
    if(sendData(socket, mailNumber, BUFFER) == -1){
        perror("SEND MAILNR error"); 
    } 

    // Check if Server answers with OK or ERR
    if(receiveFeedback(socket)){
        char line[BUFFER];
        int size = 0; 

        // Receive Mail Contents
        printf("-------------\n"); 
        do{
            if((size = recv(socket, line, BUFFER, 0)) == -1){
                perror("RECV error"); 
            }
            line[size] = '\0'; 
            printf("%s\n", line); 
        }while(strcmp(line, "."));
    }

    free(mailNumber); 

    return 0; 
}

///////////////////////////////////////////
//* DEL - FUNCTIONALITY 
///////////////////////////////////////////
int sendDelRequest(int socket){
    char* mailNumber = (char*)malloc(BUFFER); 

    // Send MailNumber
    printf("id: "); 
    fgets(mailNumber, BUFFER, stdin); 
    if(sendData(socket, mailNumber, BUFFER) == -1){
        perror("SEND MAILNR error"); 
    } 

    free(mailNumber); 

    return 0; 
}