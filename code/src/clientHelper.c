#include "../header/clientHelper.h"

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

int sendData(int socket, char* buffer, int bytesToSend){
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
