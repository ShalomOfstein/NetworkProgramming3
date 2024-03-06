#include <stdio.h> // Standard input/output library
#include <stdlib.h> // For the atoi function
#include <arpa/inet.h> // For the in_addr structure and the inet_pton function
#include <sys/socket.h> // For the socket function
#include <unistd.h> // For the close function
#include <string.h> // For the memset function
#include <time.h> // For the time function
#include <sys/time.h> // For the timeval structure
#include <netinet/tcp.h> // For the TCP_CONGESTION option
#include "RUDP_API.h" // For the RUDP API functions
#define INVALID_SOCKET -1
#define BUFFER_SIZE 1024
#define TIMEOUT_SECONDS 2
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5060
#define FILE_LENGTH 2500000



char *util_generate_random_data(unsigned int size) {
    char *buffer = NULL;
// Argument check.
    if (size == 0) {
        return NULL;
    }
    buffer = (char *) calloc(size, sizeof(char));
    if (buffer == NULL){
        printf("calloc(3) failed\n");
        return NULL;
    }
// Randomize the seed of the random number generator.
    srand(time(NULL));
    for (unsigned int i = 0; i < size; i++) {
        *(buffer + i) = ((unsigned int) rand() % 256);
    }
    return buffer;
}


int main(int args, char** argv){
    printf("RUDP Sender starting\n");
    char *server_ip = NULL;
    int server_port = 0;

    if (args != 5) {
        printf("Invalid number of arguments, inserting default values\n");
        server_ip = SERVER_IP;
        server_port = SERVER_PORT;
    } else if (args == 5) {
        server_ip = argv[2];
        server_port = atoi(argv[4]);
    }

    int sock = -1;
    struct sockaddr_in server;
    char *message = util_generate_random_data(FILE_LENGTH);

    memset(&server, 0, sizeof(server));
    sock = rudp_socket(0, 1, server, server_port, 1);
    if (sock == INVALID_SOCKET){
        perror("rudp_socket(2)");
        return 1;
    }
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port); // Todo: set the server's port

    fprintf(stdout, "Connecting to %s:%d ...\n", server_ip, server_port);
    if(rudp_connect(sock, (struct sockaddr_in *)&server, sizeof(server)) < 0){
//        perror("rudp_connect(2)");
        rudp_close(sock);
        return -1;
    }
    fprintf(stdout, "Successfully connected to the server!\n"
                    "Sending message to the server\n");
//    if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, CC_algo, strlen(CC_algo)) < 0) {
//        perror("setsockopt(2)");
//        close(sock);
//        return 1;
//    }
    int bytes_sent = rudp_sendto(sock, message, FILE_LENGTH, (struct sockaddr_in *)&server, sizeof(server));
    if (bytes_sent < 0) {
        perror("rudp_sendto(2)");
        rudp_close(sock);
        return -1;
    }
    fprintf(stdout, "Message sent successfully!\n");
    while (1){
        printf("To resend the message press 1, to exit press 0\n");
        int choice;
        if(scanf("%d", &choice)<0){
            perror("scanf(2)");
            close(sock);
            break;
        }
        if(choice == 0){
            if(rudp_dissconnect_client(sock, (struct sockaddr_in *)&server, sizeof(server)) < 0){
                perror("rudp_dissconnect_client(2)");
                rudp_close(sock);
                return -1;
            }
            printf("Disconnected from the server\n");
            rudp_close(sock);
            break;
        }
        if(choice == 1){
            bytes_sent= rudp_sendto(sock, message, FILE_LENGTH, (struct sockaddr_in *)&server, sizeof(server));
            if (bytes_sent < 0) {
                perror("rudp_sendto(2)");
                rudp_close(sock);
                return -1;
            }
            fprintf(stdout, "Message sent successfully!\n");
            bytes_sent=0;
        }
    }
    free(message);
    fprintf(stdout, "Connection closed!\n");








    return 0;
}