#include <stdio.h> // Standard input/output library
#include <stdlib.h> // For the atoi function
#include <arpa/inet.h> // For the in_addr structure and the inet_pton function
#include <sys/socket.h> // For the socket function
#include <unistd.h> // For the close function
#include <string.h> // For the memset function
#include <time.h> // For the time function
#include <sys/time.h> // For the timeval structure
#include <netinet/tcp.h> // For the TCP_CONGESTION option

#define SERVER_IP "127.0.0.1" // A default IP address for the server
#define SERVER_PORT 5060 // A default port for the server
#define MAX_CLIENTS 1 // The maximum number of clients that can be connected to the server
#define BUFFER_SIZE 1024 // The buffer size to store the received message
#define FILENAME "stats.txt" // A file to store the statistics
#define INVALID_SOCKET (-1) // define a constant for an invalid socket
#define FILE_LENGTH 2500000 // A variable to store the length of the file to send

int main(int args, char** argv){
    printf("TCP Receiver starting\n");
    int receiver_port = 0;
    char *CC_algo = 0;
    if (args != 5) {
        printf("Invalid number of arguments, inserting default values\n");
        receiver_port = SERVER_PORT;
        CC_algo = "reno";
    } else if (args ==5) {
        receiver_port = atoi(argv[2]);
        CC_algo = argv[4];
    }

    struct timeval start, end; // The timeval structures to store the start and end times.
    int sock = -1; // The variable to store the socket file descriptor.
    struct sockaddr_in server_addr; // The server's address structure.
    struct sockaddr_in client_addr; // The client's address structure.
    socklen_t client_len = sizeof(client_addr); // The client's structure length.
    // Reset the server and client structures to zeros.
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, client_len);

    // Try to create a TCP socket (IPv4, stream-based, default protocol).
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket(2)");
        return 1;
    }

//    int opt = 1;
//    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
//    {
//        perror("setsockopt(2)");
//        close(sock);
//        return 1;
//    }

    // Convert the server's address from text to binary form and store it in the server structure.
    // This should not fail if the address is valid (e.g. "127.0.0.1").
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton(3)");
        close(sock);
        return -1;
    }
    //    // Set the server's address to "0.0.0.0" (all IP addresses on the local machine).
    //    server_addr.sin_addr.s_addr = SERVER_IP;
    // Set the server's address family and port.
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(receiver_port);
    printf("receiver_port set to: %d\n", receiver_port);


    // Try to change the Congestion Control algorithm
    socklen_t lenCC = sizeof(CC_algo);
    if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, CC_algo, lenCC) != 0) {
        perror("setsockopt(2)");
        close(sock);
        return 1;
    }
    printf("CC_algo set to: %s\n", CC_algo);


    // Try to bind the socket to the server's address.
    if (bind(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("bind(2)");
        close(sock);
        return 1;
    }
    // Try to listen for incoming connections.
    if (listen(sock, MAX_CLIENTS) < 0){
        perror("listen(2)");
        close(sock);
        return 1;
    }
    fprintf(stdout, "Listening for incoming connections on port %d...\n", SERVER_PORT);




    // Try to accept a new client connection.
    int client_sock = accept(sock, (struct sockaddr *) &client_addr, &client_len);
    if (client_sock < 0) {
        perror("accept(2)");
        close(sock);
        return 1;
    }
    // Print a message to the standard output to indicate that a new client has connected.
    fprintf(stdout, "Sender %s:%d connected, beginning to receive file\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // open a file to document the stats
    FILE *stats_file = fopen(FILENAME, "w");
    if (stats_file == NULL) {
        perror("fopen");
        return 1;
    }

    int total_bytes_received = 0;
    int bytes_received=0;
    int counter = 0;
    int packages_received = 0; // The number of received packages
    double speed_sum = 0.0; // The sum of the speeds
    double time_sum = 0.0; // The sum of the elapsed times
    //create a new buffer to store the received message.
    char buffer[BUFFER_SIZE] = {0};
    memset(buffer, 0, BUFFER_SIZE);



    // The server's main loop.
    while (1) {

        // Reset the variables to their initial values.
        bytes_received=0;
        counter = 0; // the segments of the package received
        total_bytes_received = 0;

        while(total_bytes_received< FILE_LENGTH){ // 2MB (2000000 bytes)
            bytes_received = recv(client_sock, buffer, sizeof(buffer), 0);
            if(counter == 0){ // start the timer
                gettimeofday(&start, NULL);
            }
            if ((bytes_received <= 0)&&(counter==0)) { // if received an exit message (an empty packet)
                close(client_sock);
                printf("exit message received\n");
                break;
            }

            total_bytes_received = total_bytes_received+ bytes_received;
            counter++;
            memset(buffer, 0, sizeof(buffer));
        }
        gettimeofday(&end, NULL); // end the timer
        if(total_bytes_received == 0){ // if received an exit message (an empty packet)
            break;
        }
        fprintf(stdout, "File transfer complete. Recieved %d bytes \n", total_bytes_received);


        double this_elapsed_time = ((end.tv_sec - start.tv_sec) * 1000.0) + ((end.tv_usec - start.tv_usec) / 1000.0);
        double this_speed = (total_bytes_received / 1024.0 / 1024.0) / (this_elapsed_time / 1000.0);
        speed_sum = speed_sum + this_speed;
        time_sum = time_sum + this_elapsed_time;
        packages_received++;
        fprintf(stats_file, "- Run #%d Data: Time = %.2f ms; Speed = %.2f MB/s\n", packages_received, this_elapsed_time, this_speed);

        printf( "waiting for the Senders response...\n");


        // Ensure that the buffer is null-terminated, no matter what message was received.
//        // This is important to avoid SEGFAULTs when printing the buffer.
//        if (buffer[BUFFER_SIZE - 1] != '\0') {
//            buffer[BUFFER_SIZE - 1] = '\0';
//        }
        // Try to send a message to the client.
//        int bytes_sent = send(client_sock, message, message_len, 0);
//        if (bytes_sent < 0) {
//            perror("send(2)");
//            close(client_sock);
//            close(sock);
//            return 1;
//        }
        // Print the sent message.



    }
    fclose(stats_file);
    close(sock);
    printf("closed the client socket\n");
    // ... (Print statistics and times)
    printf("----------------------------------\n");
    printf("-           * Statistics *       -\n");
    stats_file = fopen(FILENAME, "r");
    if (stats_file == NULL) {
        perror("fopen");
        return 1;
    }
    while (fgets(buffer, sizeof(buffer), stats_file) != NULL) {
        printf("%s", buffer);
    }
    printf("-\n");
    printf("- Average time: %.2fms\n", time_sum / packages_received);
    printf("- Average bandwidth: %.2fMB/s\n", speed_sum / packages_received);
    printf("----------------------------------\n");
    printf( "Receiver end.\n");
    fclose(stats_file);
    return 0;
}
