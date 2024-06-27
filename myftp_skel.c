#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <err.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netdb.h>

#define BUFSIZE 512

/**
 * function: receive and analize the answer from the server
 * sd: socket descriptor
 * code: three leter numerical code to check if received, le paso el que yo quiero recibir del servidor
 * text: normally NULL but if a pointer is received as parameter
 *       then a copy of the optional message from the response
 *       is copied
 * return: result of code checking
 **/
bool recv_msg(int sd, int code, char *text) {
    char buffer[BUFSIZE], message[BUFSIZE];
    int recv_s, recv_code;

    // receive the answer
    recv_s = recv(sd, buffer, BUFSIZE-1, 0);

    // error checking
    if (recv_s < 0) warn("error receiving data\n");
    if (recv_s == 0) errx(1, "connection closed by host\n");

    buffer[recv_s] = '\0';  

    // parsing the code and message receive from the answer
    sscanf(buffer, "%d %[^\r\n]\r\n", &recv_code, message);
    printf("Server >> %d %s\n", recv_code, message);
    // optional copy of parameters
    if(text) strcpy(text, message);
    // boolean test for the code
    return (code == recv_code) ? true : false;
}

/**
 * function: send command formated to the server
 * sd: socket descriptor
 * operation: four letters (or less) command
 * param: command parameters
 **/
void send_msg(int sd, char *operation, char *param) {
    char buffer[BUFSIZE] = "";

    // command formating
    if (param != NULL){
        sprintf(buffer, "%s %s\r\n", operation, param);
    } else{
        sprintf(buffer, "%s\r\n", operation);
    }

    // send command and check for errors
    if(send(sd, buffer, strlen(buffer), 0) == -1){
        perror("sending command");
        return;
    }

}

/**
 * function: simple input from keyboard
 * return: input without ENTER key
 **/
char * read_input() {
    char *input = malloc(BUFSIZE);
    if (fgets(input, BUFSIZE, stdin)) {
        return strtok(input, "\n");
    }
    return NULL;
}

/**
 * function: login process from the client side
 * sd: socket descriptor
 **/
int authenticate(int sd) {
    char *input, desc[100];
    int code, len, rcv;

    // ask for user
    printf("username: ");
    input = read_input();

    // send the command to the server
    
    send_msg(sd, "USER", input);

    // relese memory
    free(input);

    // wait to receive password requirement and check for errors
    bool result = recv_msg(sd, 331, NULL);

    if(!result){
        perror("unexpected error on login\n");
        return 1;
    }

    // ask for password
    printf("passwd: ");
    input = read_input();   //read_input muestra la contraseña, se puede buscar una manera de que la pass no sea visible

    send_msg(sd, "PASS", input);

    // release memory
    free(input);

    // wait for answer and process it and check for errors
    result = recv_msg(sd, 230, desc);

    if(!result){
        printf("%s\n", desc);
        return 1;
    }

    return 0;
}

/**
 * function: operation get
 * sd: socket descriptor
 * file_name: file name to get from the server
 **/
void get(int sd, char *file_name) {
    char desc[BUFSIZE], buffer[BUFSIZE];
    int f_size, recv_s, r_size = BUFSIZE;
    FILE *file;

    // send the RETR command to the server
    send_msg(sd, "RETR", file_name);

    // check for the response (299 OK o 550 not found)
    if(!(recv_msg(sd, 299, NULL))){
        printf("File not found or not available: %s\n", buffer);
        return;
    }

    // parsing the file size from the answer received
    // "File %s size %ld bytes"
    sscanf(buffer, "File %*s size %d bytes", &f_size);

    // open the file to write
    file = fopen(file_name, "wb");

    if (file == NULL) {
        perror("Error opening file for writing");
        return;
    }

    //receive the file
    //leer sobre el socket y escribir en el disco hasta que termine de recibir el archivo

    while ((recv_s = recv(sd, buffer, r_size, 0)) > 0) {
        fwrite(buffer, 1, recv_s, file);
    }

    if (recv_s < 0) {
        perror("Error receiving file");
    }   

    // close the file
    fclose(file);

    // receive the OK from the server
    // codigos ftp en el servidor
     if (!recv_msg(sd, 226, NULL)) {
        perror("Error completing file transfer");
    }
}

/**
 * function: operation quit
 * sd: socket descriptor
 **/
void quit(int sd) {
    // send command QUIT to the server
    send_msg(sd, "QUIT", NULL);

    // receive the answer from the server
    if(!(recv_msg(sd, 221, NULL))){
        perror("exiting");
    }

}

/**
 * function: make all operations (get|quit)
 * sd: socket descriptor
 **/
void operate(int sd) {
    char *input, *op, *param;

    while (true) {

        printf("Client >> Operation: ");
        input = read_input();

        if (input == NULL)
            continue; // avoid empty input

        op = strtok(input, " ");            // strtok investigar: obtiene la primera parte del string hasta encontrar el separador

        // free(input);

        if (strcmp(op, "get") == 0) {       // get, quit no son comandos ftp, son comandos de usuario
            param = strtok(NULL, " ");
            get(sd, param);

        } else if (strcmp(op, "quit") == 0) {            
            quit(sd);
            break;
        } else {
            // new operations in the future
            printf("TODO: unexpected command\n");
        }
        free(input);
    }
    free(input);
}

int validateIp(char *ip) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip, &(sa.sin_addr));      // AF_INET indica ipv4, guarda en binario en sa
}

int validatePort(char *port) {
    char *endptr;
    long int value = strtol(port, &endptr, 10);
    return (*endptr == '\0' && value > 0 && value <= 65535);
}

int checkIpAndPort(int argc, char *argv[]){

    if (validateIp(argv[1]) != 1) {
        printf("Error: '%s' isn't a valid IP address\n", argv[1]);
        return 1;
    }

    if (!validatePort(argv[2])) {
        printf("Error: '%s' isn't a valid Port number.\n", argv[2]);
        return 1;
    }

    return 0;
}

/**
 * Run with
 *         ./myftp <SERVER_IP> <SERVER_PORT>
 **/
int main (int argc, char *argv[]) {

    if(argc != 3){
        perror("Expected ./myftp <SERVER_IP> <SERVER_PORT>");
        return 1;
    }

    if(checkIpAndPort(argc, argv) != 0){
        return 1;
    }

    int sd, addrinfoStatus, connectStatus;
    struct addrinfo hints;
    struct addrinfo *res, *p;
    char ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfoStatus = getaddrinfo(argv[1], argv[2], &hints, &res);

    if(addrinfoStatus != 0){
        printf("Error getaddrinfo: %s\n", gai_strerror(addrinfoStatus));
        exit(1);
    }

    for(p = res; p != NULL; p = p->ai_next){
        void *addr;

        struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
        addr = &(ipv4->sin_addr);

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf(" IP: %s\n", ipstr);
    }

    // create socket and check for errors (llenar la struct addr con la info que obtuve de argc y argv)
    // usar la documentación, ver como se instancian los 3

    // set socket data
    // agregar datos adicionales

    if((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
        perror("Creating socket \n");
        return 1;
    }

    // connect and check for errors
    // enviar la primitiva connect    

    if((connectStatus = connect(sd, res->ai_addr, res->ai_addrlen)) == -1){
        perror("Connecting to socket \n");
        return 1;
    }

    // if receive hello proceed with authenticate and operate if not warning

    if(!(recv_msg(sd, 220, NULL))){
        perror("Establishing connection\n");
        return 1;
    }

    if(authenticate(sd) != 0){
        perror("Authenticating\n");
        return 1;
    }

    operate(sd);

    // close socket
    close(sd);

    freeaddrinfo(res);

    return 0;
}