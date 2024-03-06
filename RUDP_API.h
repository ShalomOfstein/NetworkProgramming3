#include <stdio.h> // Standard input/output library
#include <stdlib.h> // For the atoi function
#include <arpa/inet.h> // For the in_addr structure and the inet_pton function
#include <sys/socket.h> // For the socket function
#include <unistd.h> // For the close function
#include <string.h> // For the memset function
#include <time.h> // For the time function
#include <sys/time.h> // For the timeval structure
#include <netinet/tcp.h> // For the TCP_CONGESTION option
//
// Created by shalom on 2/29/24.
//

#ifndef NETWORK3_RUDP_API_H
#define NETWORK3_RUDP_API_H
#define SYN_FLAG 0x1
#define ACK_FLAG 0x2
#define FIN_FLAG 0x4
#define SYN_ACK_FLAG 0x3
#define FIN_ACK_FLAG 0x6
#define BUFFER_SIZE 1024


typedef struct _RUDP_PACKET {
    int length;
    int checksum;
    int seq_num;
    int flags;
    char data[BUFFER_SIZE];
}RUDP_PACKET;


//typedef struct _RUDP_SOCKET {
//    int is_server;
//    int is_connected;
//    struct sockaddr_in destination_addr;
//}RUDP_SOCKET;


int rudp_my_socket();
int rudp_my_send(int sockfd, const RUDP_PACKET *rudp_packet);
int rudp_my_sendto(int sockfd, const RUDP_PACKET *rudp_packet, struct sockaddr_in *dest_addr, socklen_t addrlen);
int rudp_my_recv(int sockfd, RUDP_PACKET *rudp_packet);
int rudp_my_recvfrom(int sockfd, RUDP_PACKET *rudp_packet, struct sockaddr_in *src_addr, socklen_t *addrlen);
int send_ack(int sockfd, struct sockaddr_in *src_addr, socklen_t addrlen, int seq_num);
int rudp_connect(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen);
int rudp_accept(int sockfd, struct sockaddr_in *src_addr, socklen_t *addrlen);
int rudp_dissconnect_client(int sockfd, struct sockaddr_in *dest_addr, socklen_t addrlen);
int rudp_dissconect_server(int sockfd, struct sockaddr_in *src_addr, socklen_t addrlen);
int rudp_recvfrom(int sock, struct sockaddr_in *src_addr, socklen_t *addrlen);
int rudp_sendto(int sock, const void *data, size_t len, struct sockaddr_in *dest_addr, socklen_t addrlen);
int rudp_socket(int server, int opt, struct sockaddr_in server_addr,int server_port, int MAX_CLIENTS);
int rudp_close(int sock);





#endif //NETWORK3_RUDP_API_H
