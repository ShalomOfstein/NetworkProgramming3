#include <stdio.h> // Standard input/output library
#include <stdlib.h> // For the calloc and atoi function
#include <arpa/inet.h> // For the in_addr structure and the inet_pton function
#include <sys/socket.h> // For the socket function
#include <unistd.h> // For the close function
#include <string.h> // For the memset function
#include <time.h> // For the time function
#include <netinet/tcp.h> // For the TCP_CONGESTION option



#define SERVER_IP "127.0.0.1" // A default IP address for the server
#define SERVER_PORT 5060 // A default port for the server
#define BUFFER_SIZE 1024 // The buffer size to store the received message
#define INVALID_SOCKET (-1) // define a constant for an invalid socket
#define FILE_LENGTH 2500000 // A variable to store the length of the file to send

/*
* @brief A random data generator function based on srand() and rand().
* @param size The size of the data to generate (up to 2^32 bytes).
* @return A pointer to the buffer.
*/
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

int main(int args, char** argv) {
    printf("TCP Sender starting\n");
    char *receiver_ip = NULL;
    int receiver_port = 0;
    char *CC_algo = 0;

    if (args != 7) {
        printf("Invalid number of arguments, inserting default values\n");
        receiver_ip = SERVER_IP;
        receiver_port = SERVER_PORT;
        CC_algo = "reno";
    } else if (args == 7) {
        receiver_ip = argv[2];
        receiver_port = atoi(argv[4]);
        CC_algo = argv[6];
    }
    // The variable to store the socket file descriptor, and the server's address.
    int sock = INVALID_SOCKET;
    struct sockaddr_in server_addr;
    // Create a message to send to the server.
    char *content = util_generate_random_data(FILE_LENGTH); // generate 2 MB of data
    int content_len = FILE_LENGTH;
    // Reset the server structure to zeros.
    memset(&server_addr, 0, sizeof(server_addr));


    // Try to create a TCP socket (IPv4, stream-based, default protocol).
    sock = socket(AF_INET, SOCK_STREAM, 0);
    // If the socket creation failed, print an error message and return 1.
    if ((sock) == INVALID_SOCKET) {
        perror("socket(2)");
        free(content);
        return -1;
    }
    // Convert the server's address from text to binary form and store it in the server structure.
    // This should not fail if the address is valid (e.g. "127.0.0.1").
    if (inet_pton(AF_INET, receiver_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton(3)");
        close(sock);
        free(content);
        return -1;
    }
    // Set the server's address family, port, and address.
    server_addr.sin_family = AF_INET; //IPv4
    server_addr.sin_port = htons(receiver_port); // set the server's port

    //try to change the Congestion Control algorithm
    if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, CC_algo, strlen(CC_algo)) < 0) {
        perror("setsockopt(2)");
        close(sock);
        free(content);
        return -1;
    }
    printf("CC_algo set to: %s\n", CC_algo);

    printf( "Connecting to %s : %d\n", receiver_ip, receiver_port);
    // Try to connect to the server.
    if (connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect(2)");
        close(sock);
        free(content);
        return -1;
    }
    fprintf(stdout, "Successfully connected to the receiver!\n"
                    "Sending message to the receiver.\n");



    // Send the message to the server.
    int bytes_sent = send(sock, content, content_len, 0);
    // If the message sending failed, print an error message and return 1.
    if (bytes_sent <= 0) {
        perror("send(2)");
        close(sock);
        free(content);
        return -1;
    }
    fprintf(stdout, "Sent %d bytes to the receiver successfully\n",bytes_sent);
//    // Try to receive a message from the server.
//    int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
//    // If the message receiving failed, print an error message and return 1.
//    if (bytes_received < 0) {
//        perror("recv(2)");
//        close(sock);
//        return 1;
//    }
//    // Ensure that the buffer is null-terminated, no matter what message was received.
//    // This is important to avoid SEGFAULTs when printing the buffer.
//    if (buffer[BUFFER_SIZE - 1] != '\0'){
//        buffer[BUFFER_SIZE - 1] = '\0';
//    }
//    fprintf(stdout, "Received %d bytes from the server: %s\n", bytes_received, buffer);

    while(1) {

        printf("To resend the message press 1, to exit press 0\n");
        int choice;
        if (scanf("%d", &choice) < 0) {
            perror("scanf(2)");
            close(sock);
            break;
        }
        if (choice == 0) {
            int exit_bytes_sent = send(sock, NULL, 0, 0);
            // If the message sending failed, print an error message and return 1.
            if (exit_bytes_sent < 0) {
                perror("send(2)");
                close(sock);
                free(content);
                return -1;
            }
            printf("Exiting Program now\n");
            break;
        }
        if (choice == 1) {
            // Send the message to the server.
            bytes_sent = 0;
            bytes_sent = send(sock, content, content_len, 0);
            // If the message sending failed, print an error message and return 1.
            if (bytes_sent < 0) {
                perror("send(2)");
                close(sock);
                free(content);
                return -1;
            }
            fprintf(stdout, "Sent %d bytes to the server!\n", bytes_sent);
        }
    }


//            // Try to receive a message from the server.
//            bytes_received=0;
//            bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
//            // If the message receiving failed, print an error message and return 1.
//            if (bytes_received < 0) {
//                perror("recv(2)");
//                close(sock);
//                return 1;
//            }
//            // Ensure that the buffer is null-terminated, no matter what message was received.
//            // This is important to avoid SEGFAULTs when printing the buffer.
//            if (buffer[BUFFER_SIZE - 1] != '\0'){
//                buffer[BUFFER_SIZE - 1] = '\0';
//            }
//            fprintf(stdout, "Received %d bytes from the server: %s\n", bytes_received, buffer);
//        }
//    }




    // Close the socket with the server.
    close(sock);
    free(content);

    fprintf(stdout, "Connection closed!\n");

    // Return 0 to indicate that the client ran successfully.
    return 0;
}