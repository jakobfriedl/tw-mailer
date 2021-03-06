#include "../header/helper.h"

ssize_t my_read(int socketDecriptor, char *ptr){
    static int read_cnt = 0;
    static char *read_ptr;
    static char read_buf[BUFFER];

    if (read_cnt <= 0){
    again:
        if((read_cnt = read(socketDecriptor, read_buf, sizeof(read_buf))) < 0){
            if (errno == EINTR)
                goto again;
            return -1;
        }else if (read_cnt == 0)
            return 0;
        read_ptr = read_buf;
    }
    read_cnt--;

    *ptr = *read_ptr++;

    return 1;
}

ssize_t readline(int socketDecriptor, void *buffer, size_t maxlen){
    ssize_t n, rc;
    char c, *ptr;
    ptr = buffer;

    for(n = 1; n < (int)maxlen; n++){
        if((rc = my_read(socketDecriptor, &c)) == 1){
            *ptr++ = c;
            if (c == '\n')
                break;      // newline is stored
        }else if(rc == 0){
            if (n == 1)
                return 0;   // EOF, no data read
            else
                break;      // EOF, some data was read
        }else
            return -1;      // error, errno set by read() in my_read()
    }
    *ptr = 0;               // null terminate

    return n; 
}

ssize_t writen(int socketDescriptor, const void *buffer, size_t n){
    size_t bytesLeft;
    ssize_t bytesWritten;

    const char *ptr;
    ptr = buffer;

    bytesLeft = n;
    while (bytesLeft > 0){
        if ((bytesWritten = write(socketDescriptor, ptr, bytesLeft)) <= 0){
            if (errno == EINTR)
                bytesWritten = 0; // and call write() again
            else
                return -1;
        }
        bytesLeft -= bytesWritten;
        ptr += bytesWritten;
    }
    return n;
}

int validateUserName(char* username){
    // Check if username is max. 8 characters long
    if(strlen(username) > MAX_USERNAME_LENGTH || strlen(username) < MIN_USERNAME_LENGTH){
        printf("[ ! ] username has invalid length.\n");
        return 0; 
    }

    // Check if username only contains letters and numbers
    for(int i = 0; i <= (int)strlen(username)-1; i++){
        if((username[i] >= 'a' && username[i] <= 'z')
        || (username[i] >= '0' && username[i] <= '9')){
            // Skip character
        }else{
            printf("[ ! ] username contains invalid characters.\n"); 
            return 0; 
        }
    }
    return 1; 
}
