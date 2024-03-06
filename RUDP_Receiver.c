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
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5060
#define MAX_CLIENTS 1
#define MAX_RUNS 50
#define FILE_LENGTH 2500000
#define FILENAME "stats.txt"


int main(int args, char** argv){
    printf("RUDP Receiver starting\n");
    int server_port = 0;
    if (args != 3) {
        printf("Invalid number of arguments, inserting default values\n");
        server_port = SERVER_PORT;
    } else if (args ==3) {
        server_port = atoi(argv[2]);

    }

    struct timeval start, end;
    int sock = -1;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Reset the server and client structures to zeros.
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, client_len);

    // The variable to store the socket option for reusing the server's address.
    int opt = 1;

    sock = rudp_socket(1,opt, server_addr,server_port,MAX_CLIENTS);
    if (sock == INVALID_SOCKET) {
        perror("rudp_socket_setup(2)");
        return 1;
    }
    int client_sock = rudp_accept(sock, (struct sockaddr_in *)&client_addr, &client_len);
    if (client_sock == INVALID_SOCKET){
        perror("rudp_accept(2)");
        close(sock);
        return 1;
    }
    printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

//    socklen_t len = sizeof(CC_algo);
//    // Try to change the Congestion Control algorithm
//    if (setsockopt(client_sock, IPPROTO_TCP, TCP_CONGESTION, CC_algo, len) != 0) {
//        perror("setsockopt(2)");
//        close(client_sock);
//        return 1;
//    }

    int total_bytes_received = 0;
    int bytes_received=0;
    int counter = 0;
    int packages_received = 0;
    double speed_sum = 0.0;
    double time_sum = 0.0;
    char buffer[BUFFER_SIZE];

    FILE *stats_file = fopen(FILENAME, "w");
    if (stats_file == NULL) {
        perror("fopen");
        return 1;
    }
    RUDP_PACKET *packet = (RUDP_PACKET *) malloc(sizeof(RUDP_PACKET));// is this the right way to do it?
    while(1){

        bytes_received=0;
        counter = 0;
        total_bytes_received = 0;
        memset(buffer, 0, BUFFER_SIZE);
        int sequence =-1;


        while(total_bytes_received<FILE_LENGTH){
            bytes_received=0;
            bytes_received = rudp_recvfrom(sock,packet,(struct sockaddr_in *)&client_addr, &client_len);
            if(packet->seq_num > sequence||packet->seq_num ==-1) {
                if (bytes_received == -1) {
                    perror("rudp_recvfrom(2)");
                    close(sock);
                    free(packet);
                    return -1;
                }
                if (counter == 0) {
                    gettimeofday(&start, NULL);
                    printf("Started receiving file...\n");
                }
                if (bytes_received == -2) {
                    close(sock);
                    break;
                }
                sequence ++;
                total_bytes_received = total_bytes_received + bytes_received;
                counter++;
//                printf("including packet %d, Received total of %d bytes from the client\n", packet->seq_num,total_bytes_received);
            }
            else{
//                printf("Received a duplicate packet, ignoring it\n");
            }
            if((counter%50==0)&&(counter!=0)){
                printf(" -> Received %d packets from the sender\n",counter);
            }
            memset(buffer, 0, sizeof(buffer));
            memset(packet, 0, sizeof(RUDP_PACKET));
        }

        gettimeofday(&end, NULL);
        if(bytes_received==-2){
            break;
        }


        printf("File transfer complete. Received a total of %d bytes\n",total_bytes_received);
        double this_elapsed_time = ((end.tv_sec - start.tv_sec) * 1000.0) + ((end.tv_usec - start.tv_usec) / 1000.0);
        double this_speed = (total_bytes_received / 1024.0 / 1024.0) / (this_elapsed_time / 1000.0);
        speed_sum = speed_sum + this_speed;
        time_sum = time_sum + this_elapsed_time;

        packages_received++;

        fprintf(stats_file, "- Run #%d Data: Time = %.2f ms; Speed = %.2f MB/s\n", packages_received, this_elapsed_time, this_speed);

        // Print the sent message.
        printf( "waiting for the Senders response...\n");

    }
    free(packet);
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

    return 0;

}