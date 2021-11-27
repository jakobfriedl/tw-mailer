#include "../header/clientCommands.h"

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