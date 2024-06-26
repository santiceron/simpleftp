#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <err.h>

#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netdb.h>
#include <arpa/inet.h>

#define BUFSIZE 512
#define CMDSIZE 4
#define PARSIZE 100

#define USERNAME "santiago"
#define PASSWORD "123"

#define MSG_220 "220 srvFtp version 1.0\r\n"
#define MSG_221 "221 Goodbye\r\n"
#define MSG_226 "226 Transfer complete\r\n"
#define MSG_230 "230 User %s logged in\r\n"
#define MSG_299 "299 File %s size %ld bytes\r\n"
#define MSG_331 "331 Password required for %s\r\n"
#define MSG_530 "530 Login incorrect\r\n"
#define MSG_550 "550 %s: no such file or directory\r\n"



/**
 * function: receive the commands from the client
 * sd: socket descriptor
 * operation: \0 if you want to know the operation received
 *            OP if you want to check an especific operation
 *            ex: recv_cmd(sd, "USER", param)
 * param: parameters for the operation involve
 * return: only usefull if you want to check an operation
 *         ex: for login you need the seq USER PASS
 *             you can check if you receive first USER
 *             and then check if you receive PASS
 **/
bool recv_cmd(int sd, char *operation, char *param) {
    char buffer[BUFSIZE], *token;
    int recv_s;

    // receive the command in the buffer and check for errors
    recv_s = recv(sd, buffer, BUFSIZE-1, 0);

    if (recv_s < 0) warn("error receiving data");
    if (recv_s == 0) errx(1, "connection closed by host");


    // expunge the terminator characters from the buffer
    buffer[strcspn(buffer, "\r\n")] = 0;

    // complex parsing of the buffer
    // extract command receive in operation if not set \0
    // extract parameters of the operation in param if it needed
    token = strtok(buffer, " ");
    if (token == NULL || strlen(token) < 4) {
        warn("not valid ftp command");
        return false;
    } else {
        if (operation[0] == '\0') strcpy(operation, token);
        if (strcmp(operation, token)) {
            warn("abnormal client flow: did not send %s command", operation);
            return false;
        }
        token = strtok(NULL, " ");
        if (token != NULL) strcpy(param, token);
    }
    return true;
}

/**
 * function: send answer to the client
 * sd: file descriptor
 * message: formatting string in printf format
 * ...: variable arguments for economics of formats
 * return: true if not problem arise or else
 * notes: the MSG_x have preformated for these use
 **/
bool send_ans(int sd, char *message, ...){
    char buffer[BUFSIZE];

    va_list args;
    va_start(args, message);

    vsprintf(buffer, message, args);
    va_end(args);

    // send answer preformated and check errors
    if(send(sd, buffer, BUFSIZE-1, 0) == -1){
        perror("sending answer to client\n");
        return false;
    }

    return true;
}

/**
 * function: RETR operation
 * sd: socket descriptor
 * file_path: name of the RETR file
 **/

void retr(int sd, char *file_path) {
    FILE *file;    
    int bread;
    long fsize;
    char buffer[BUFSIZE];
    char cwd[1024];

    getcwd(cwd, sizeof(cwd));
    printf("Directorio actual: %s\n", cwd);

    // check if file exists if not inform error to client

    // send a success message with the file length

    // important delay for avoid problems with buffer size
    sleep(1);

    // send the file

    // close the file

    // send a completed transfer message
}
/**
 * funcion: check valid credentials in ftpusers file
 * user: login user name
 * pass: user password
 * return: true if found or false if not
 **/
bool check_credentials(char *user, char *pass) {
    FILE *file;
    char *path = "./ftpusers.txt", *line = NULL, credentials[100];
    size_t line_size = 0;
    bool found = false;

    // make the credential string
    sprintf(credentials, "%s:%s", user, pass);

    // check if ftpusers file it's present
    if ((file = fopen(path, "r"))==NULL) {
        warn("Error opening %s", path);
        return false;
    }

    // search for credential string
    while (getline(&line, &line_size, file) != -1) {
        strtok(line, "\n");
        if (strcmp(line, credentials) == 0) {
            found = true;
            break;
        }
    }

    // close file and release any pointers if necessary
    fclose(file);
    if (line) free(line);

    // return search status
    return found;
}

/**
 * function: login process management
 * sd: socket descriptor
 * return: true if login is succesfully, false if not
 **/
bool authenticate(int sd) {
    char user[PARSIZE], pass[PARSIZE];

    char *passwordReq = "password";
    int len = strlen(passwordReq);
    int rcv;
    bool result;

    // wait to receive USER action

    if(!(recv_cmd(sd, "USER", user))){
        send_ans(sd, MSG_530);
        return false;
    }

    // ask for password
    if(!(send_ans(sd, MSG_331))){
        perror("unexpected error requesting password");
        return false;
    }

    // wait to receive PASS action
    if(!(recv_cmd(sd, "PASS", pass))){
        send_ans(sd, MSG_530);
        return false;
    }

    // if credentials don't check denied login
    // confirm login

    if(check_credentials(user, pass)){
        send_ans(sd, MSG_230, user);
        return true;
    } else {
        send_ans(sd, MSG_530);
        return false;
    }
}

/**
 *  function: execute all commands (RETR|QUIT)
 *  sd: socket descriptor
 **/

void operate(int sd) {
    char op[CMDSIZE], param[PARSIZE];

    while (true) {
        op[0] = param[0] = '\0';
        // check for commands send by the client if not inform and exit
        
        bool success = recv_cmd(sd, op, param);

        if(!success){
            printf("Error receiving command\n");
            continue;
        }

        if (strcmp(op, "RETR") == 0) {
            printf("%s\n", param);
            retr(sd, param);
        } else if (strcmp(op, "QUIT") == 0) {
            // send goodbye and close connection
            send_ans(sd, MSG_221);

            break;
        } else {
            // invalid command
            // furute use
        }
    }
}

/**
 * Run with
 *         ./mysrv <SERVER_PORT>
 **/
int main (int argc, char *argv[]) {

    // arguments checking
    if (argc < 2) {
        errx(1, "Port expected as argument");
    } else if (argc > 2) {
        errx(1, "Too many arguments");
    }

    // reserve sockets and variables space
    int master_sd, slave_sd, addrinfoStatus, bindStatus, listenStatus;
    int yes=1;
    struct addrinfo hints, *res, *p;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    char s[INET6_ADDRSTRLEN];

    // first, load up address structs with getaddrinfo():

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // use IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    if((addrinfoStatus = getaddrinfo(NULL, argv[1], &hints, &res)) != 0){
        printf("getaddrinfo error: %s\n", gai_strerror(addrinfoStatus));
        exit(1);
    }

    // loop through all the results and bind to the first we can
    for(p = res; p != NULL; p = p->ai_next) {

        // make a socket:
        // create server socket and check errors
        // creating socket, if error go to the next addr
        if ((master_sd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        // for making sure that the port is free, if error exit
        if (setsockopt(master_sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        
        // binding master socket, if error go to the next addr
        if (bind(master_sd, p->ai_addr, p->ai_addrlen) == -1) {
            close(master_sd);
            perror("server: bind");
            continue;
        }

        // created socket and bounded succesfully
        break;
    }

    freeaddrinfo(res); // all done with this structure

    if (p == NULL) {
        perror("server: failed to bind\n");
        exit(1);
    }

    // make it listen
    if((listenStatus = listen(master_sd, 5)) == -1){
        printf("Error setting listen \n");
        return 1;
    }

    printf("server: waiting for connections...\n");

    // main loop
    while(1){

        addr_size = sizeof their_addr;

        // accept connectiones sequentially and check errors
        slave_sd = accept(master_sd, (struct sockaddr *)&their_addr, &addr_size);

        if(slave_sd == -1){
            perror("accept");
            continue;
        }

        if(!fork()){
            // slave socket code, don't need master_sd
            close(master_sd);

            // send hello
            if(send(slave_sd, MSG_220, strlen(MSG_220), 0) == -1){
                perror("sending hello\n");
                return 1;
            }

            // operate only if authenticate is true
            if(authenticate(slave_sd)){
                printf("Login succesful\n");
                operate(slave_sd);
            } else{
                printf("Wrong username or password\n");
                close(slave_sd);
                exit(0);
            }

            
            close(slave_sd);
            break;
        }

        // master socket code, don't need slave_sd
        close(slave_sd);
    }

    // close server socket

    close(master_sd);

    return 0;
}
