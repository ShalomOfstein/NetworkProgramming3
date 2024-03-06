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
//**********************************************************************************************************
//checksum function
/*
* @brief A checksum function that returns 16 bit checksum for data.
* @param data The data to do the checksum for.
* @param bytes The length of the data in bytes.
* @return The checksum itself as 16 bit unsigned number.
* @note This function is taken from RFC1071, can be found here:
* @note https://tools.ietf.org/html/rfc1071
* @note It is the simplest way to calculate a checksum and is not very strong.
* However, it is good enough for this assignment.
* @note You are free to use any other checksum function as well.
* You can also use this function as such without any change.
*/

unsigned short int calculate_checksum(void *data, unsigned int bytes) {
    unsigned short int *data_pointer = (unsigned short int *) data;
    unsigned int total_sum = 0;
// Main summing loop
    while (bytes > 1) {
        total_sum += *data_pointer++;
        bytes -= 2;
    }
// Add left-over byte, if any
    if (bytes > 0) {
        total_sum += *((unsigned char *) data_pointer);
    }
// Fold 32-bit sum to 16 bits
    while (total_sum >> 16) {
        total_sum = (total_sum & 0xFFFF) + (total_sum >> 16);
    }
    return (~((unsigned short int) total_sum));
}

//**********************************************************************************************************

// basic functions I will use to create more complex functions
int rudp_my_socket(){
    return socket(AF_INET, SOCK_DGRAM, 0);
}

int rudp_my_send(int sockfd, const RUDP_PACKET *rudp_packet){
    return send(sockfd, (const char*)rudp_packet, sizeof(rudp_packet)+rudp_packet->length, 0);
}
int rudp_my_sendto(int sockfd, const RUDP_PACKET *rudp_packet, struct sockaddr_in *dest_addr, socklen_t addrlen){
    return sendto(sockfd, (const char*)rudp_packet, sizeof(RUDP_PACKET)+rudp_packet->length, 0, (struct sockaddr*)dest_addr, addrlen);
}

int rudp_my_recv(int sockfd, RUDP_PACKET *rudp_packet){
    return recv(sockfd, rudp_packet, sizeof(rudp_packet)+rudp_packet->length, 0);
}
int rudp_my_recvfrom(int sockfd, RUDP_PACKET *rudp_packet, struct sockaddr_in *src_addr, socklen_t *addrlen){
    return recvfrom(sockfd, rudp_packet, sizeof(RUDP_PACKET)+rudp_packet->length, 0, (struct sockaddr*)src_addr, addrlen);
}

//**********************************************************************************************************

// The function to send an ACK packet.
int send_ack(int sockfd, struct sockaddr_in *src_addr, socklen_t addrlen, int seq_num){
    RUDP_PACKET ack_packet;
    ack_packet.length = 0; // no data in ack packet
    ack_packet.checksum = 0;
    ack_packet.seq_num = seq_num; // the squence number of the received packet
    ack_packet.flags = ACK_FLAG;


    if(rudp_my_sendto(sockfd, &ack_packet,src_addr, addrlen) < 0){
        perror("sendACK failed");
        return -1;
    }
    return 0;
}
//**********************************************************************************************************

// The function to set up a connection with the server
int rudp_connect(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen){

    RUDP_PACKET syn_packet;
    RUDP_PACKET syn_ack_packet;


    //make the syn packet
    syn_packet.length = 0; // no data i syn packet
    syn_packet.checksum = 0;
    syn_packet.seq_num = 0; // only 1 packet so seq_num is 0
    syn_packet.flags = SYN_FLAG;


    if(rudp_my_sendto(sockfd, &syn_packet, dest_addr, addrlen) < 0){
        perror("sendto failed");
        return -1;
    }
    printf("Sent SYN packet\n");
    // TODO: set up a timeout

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SECONDS;
    timeout.tv_usec = 0;

    // Set up the file descriptor set for select
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);

    // Use select to wait for data on the socket or timeout
    int ready = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

    if (ready < 0) {
        perror("select failed");
        return -1;
    } else if (ready == 0) {
        // Timeout occurred
        printf("Timeout occurred. Connection failed.\n");
        return -1;
    } else {
        // Data is ready to read
        if (rudp_my_recvfrom(sockfd, &syn_ack_packet, dest_addr, &addrlen) < 0) {
            perror("recvfrom failed");
            return -1;
        }

        printf("Received SYNACK packet\n");

        if (syn_ack_packet.flags == SYN_ACK_FLAG) {
            // Make Ack packet
            if (send_ack(sockfd, dest_addr, addrlen, 0) < 0) {
                perror("sendto failed");
                return -1;
            }
            printf("Sent ACK packet\n");
            return 0;
        } else {
            printf("Invalid SYNACK packet\n");
            return -1;
        }
    }

//    if(rudp_my_recvfrom(sockfd, &syn_ack_packet, dest_addr, &addrlen) < 0){
//        perror("recvfrom failed");
//        return -1;
//    }
//    printf("Received SYNACK packet\n");
//
//    if(syn_ack_packet.flags == SYN_ACK_FLAG){
//        // make Ack packet
//        if(send_ack(sockfd,dest_addr,addrlen, 0) < 0){
//            perror("sendto failed");
//            return -1;
//        }
//        printf("Sent ACK packet\n");
//        return 0;
//    }else{
//        printf("Invalid SYNACK packet\n");
//        return -1;
//    }
}

// The function to accept a connection from a client
int rudp_accept(int sockfd, struct sockaddr_in *src_addr, socklen_t *addrlen) {
    RUDP_PACKET syn_packet;
    RUDP_PACKET syn_ack_packet;
    RUDP_PACKET ack_packet;

    if (rudp_my_recvfrom(sockfd, &syn_packet, src_addr, addrlen) < 0) {
        perror("recvfrom failed");
        return -1;
    }

    if (syn_packet.flags == SYN_FLAG) {
        printf("Received SYN packet\n");

        //make the syn_ack packet
        syn_ack_packet.length = 0; // no data i syn_ack packet
        syn_ack_packet.checksum = 0;
        syn_ack_packet.seq_num = 0; // only 1 packet so seq_num is 0
        syn_ack_packet.flags = SYN_ACK_FLAG;


        if (rudp_my_sendto(sockfd, &syn_ack_packet, src_addr, *addrlen) < 0) {
            perror("sendto failed");
            return -1;
        }
        printf("Sent SYNACK packet\n");

        if (rudp_my_recvfrom(sockfd, &ack_packet, src_addr, addrlen) < 0) {
            perror("recvfrom failed");
            return -1;
        }


        if (ack_packet.flags == ACK_FLAG) {
            printf("Received ACK packet\n");
            return 0;
        } else {
            printf("Invalid ACK packet\n");
            return -1;
        }
    } else {
        printf("Invalid SYN packet\n");
        return -1;
    }
}
//**********************************************************************************************************
// The function to close the connection by the client
int rudp_dissconnect_client(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen){
    RUDP_PACKET fin_packet;
    RUDP_PACKET fin_ack_packet;

    //make the fin packet
    fin_packet.length = 0; // no data i fin packet
    fin_packet.checksum = 0;
    fin_packet.seq_num = 0; // only 1 packet so seq_num is 0
    fin_packet.flags = FIN_FLAG;

    if(rudp_my_sendto(sockfd, &fin_packet,dest_addr,addrlen) < 0){
        perror("sendto failed");
        return -1;
    }
    printf("Disconnecting from server, sending FIN packet\n");

    if(rudp_my_recvfrom(sockfd, &fin_ack_packet,dest_addr, &addrlen) < 0){
        perror("recvfrom failed");
        return -1;
    }

    printf("Received FINACK packet\n");

    if(fin_ack_packet.flags == FIN_ACK_FLAG){
        // send Ack packet
        if(send_ack(sockfd,dest_addr,addrlen,0) < 0){
            perror("sendto failed");
            return -1;
        }
        printf("Sent ACK packet\n");
        return 0;
    }else{
        printf("Invalid FINACK packet\n");
        return -1;

    }
}

int rudp_dissconect_server(int sockfd, struct sockaddr_in *src_addr, socklen_t addrlen){
    RUDP_PACKET fin_ack_packet;
    RUDP_PACKET ack_packet;

    //make the fin_ack packet
    fin_ack_packet.length = 0; // no data i fin_ack packet
    fin_ack_packet.checksum = 0;
    fin_ack_packet.seq_num = 0; // only 1 packet so seq_num is 0
    fin_ack_packet.flags = FIN_ACK_FLAG;


    if(rudp_my_sendto(sockfd, &fin_ack_packet, src_addr, addrlen) < 0){
        perror("FIN_ACK failed");
        return -1;
    }
    printf("Sent FINACK packet\n");


    if(rudp_my_recvfrom(sockfd, &ack_packet, src_addr, &addrlen) < 0){
        perror("recvfrom failed");
        return -1;
    }
    printf("Received ACK packet\n");
    return 0;
}

//**********************************************************************************************************

int rudp_recvfrom(int sock, struct sockaddr_in *src_addr, socklen_t *addrlen){
    RUDP_PACKET rudp_packet;
    int receive_bytes = rudp_my_recvfrom(sock, &rudp_packet, src_addr, addrlen);
    if(receive_bytes < 0){
        perror("recv failed");
        return -1;
    }

    // regular send pack - always return an ACK paket
    if(rudp_packet.flags == 0) {
        int seq_num = rudp_packet.seq_num;

        if (rudp_packet.data == NULL) {
            fprintf(stderr, "Error: Received NULL data.\n");
            return -1;
        }
        int checksum = calculate_checksum(rudp_packet.data, rudp_packet.length);
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        if(checksum == rudp_packet.checksum){
            if(seq_num>=0){
//                printf("Received packet number %d. Checksum = True. sending ACK for packet...\n", seq_num);
                send_ack(sock, src_addr, *addrlen, seq_num);
                memset(buffer, 0, rudp_packet.length);

            } else{
                printf("Received last packet!\n");
                send_ack(sock, src_addr, *addrlen, seq_num);
                memset(buffer, 0, rudp_packet.length);
                return rudp_packet.length;
            }
        } else{
            printf("Checksum is incorrect\n");
            send_ack(sock, src_addr, *addrlen, seq_num-1);
        }
    }
    // if the packet is a FIN packet
    else if(rudp_packet.flags==FIN_FLAG){
        printf("FIN packet received. Disconnecting...\n");
        if(rudp_dissconect_server(sock, src_addr, *addrlen) < 0){
            perror("dissconect failed");
            return -1;
        }
        printf("Disconnected from the client\n");
        return -2;
    }
    else{
        printf("Invalid packet\n");
        return -1;
    }
    return rudp_packet.length;
}


int rudp_sendto(int sock, const void *data, size_t len, struct sockaddr_in *dest_addr, socklen_t addrlen){
    RUDP_PACKET rudp_packet;
    size_t num_packets = (len + BUFFER_SIZE - 1) / BUFFER_SIZE;
    char* data_ptr = (char*)data;

    for (size_t i = 0; i < num_packets; i++) {
        data_ptr = (char*)data + (i * BUFFER_SIZE);
        rudp_packet.seq_num = i;
        rudp_packet.flags = 0;
        rudp_packet.checksum = calculate_checksum(data_ptr, BUFFER_SIZE);
//        rudp_packet.data = buffer;
        memcpy(rudp_packet.data, data_ptr, BUFFER_SIZE);
        rudp_packet.length = BUFFER_SIZE;
        if (i == num_packets - 1) {
            rudp_packet.seq_num = -1;
            rudp_packet.length = len - i * BUFFER_SIZE;
        }
        if (rudp_my_sendto(sock, &rudp_packet, dest_addr, addrlen) < 0) {
            perror("sendto failed");
            return -1;
        }
//        printf("Packet %zu out of %zu sent\n", i , num_packets-1);

        // Set up a timeout
        struct timeval timeout;
        timeout.tv_sec = TIMEOUT_SECONDS;
        timeout.tv_usec = 0;

        while (1) {
            // Set up the file descriptor set for select
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(sock, &read_fds);

            // Use select to wait for data on the socket or timeout
            int ready = select(sock + 1, &read_fds, NULL, NULL, &timeout);

            if (ready < 0) {
                perror("select failed");
                return -1;
            } else if (ready == 0) {
                // Timeout occurred, resend the packet
                printf("Timeout occurred, resending packet\n");
                if (rudp_my_sendto(sock, &rudp_packet, dest_addr, addrlen) < 0) {
                    perror("sendto failed");
                    return -1;
                }
                printf("Resent packet, waiting for ACK\n");
                timeout.tv_sec = TIMEOUT_SECONDS;
                timeout.tv_usec = 0;
                continue;
            } else {
                // Data is ready to read, check if it's an ACK packet
                RUDP_PACKET received_packet;
                if (rudp_my_recvfrom(sock, &received_packet, dest_addr, &addrlen)){

                    if ((received_packet.flags == ACK_FLAG)&&(received_packet.seq_num == rudp_packet.seq_num )){
                        // ACK received, break the loop and continue to the next packet
                        if (received_packet.seq_num != -1) {
//                            printf("Received ACK for packet number %d\n", received_packet.seq_num);
                        }else{
                            printf("Received ACK for last packet\n");
                        }
                    }
                    break;
                }
            }

        }

    }
    printf("All data sent!\n");
    return 0;
}




//**********************************************************************************************************
// The RUDP's API socket function.
int rudp_socket(int server, int opt, struct sockaddr_in server_addr,int server_port, int MAX_CLIENTS){
    if(server){
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock == INVALID_SOCKET){
            perror("rudp_socket(2)");
            return -1;
        }

        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            perror("setsockopt(2)");
            close(sock);
            return -1;
        }


        // Set the server's address to "0.0.0.0" (all IP addresses on the local machine).
        server_addr.sin_addr.s_addr = INADDR_ANY;
        // Set the server's address family and port.
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);

        // Try to bind the socket to the server's address.
        if (bind(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
            perror("bind(2)");
            close(sock);
            return -1;
        }

        fprintf(stdout, "Listening for incoming connections on port %d...\n", server_port);
        return sock;
    }else {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock == INVALID_SOCKET) {
            perror("rudp_socket(2)");
            return -1;
        }
        return sock;
    }
}
// The RUDP's API receive function.



int rudp_close(int sock){
    return close(sock);
}


