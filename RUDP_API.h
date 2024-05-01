#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>


#define TIMES_TO_SEND 100
#define BITS_TO_BYTES 8

#define TIMEOUT_MICROSECS 700000
#define TIMEOUT_SECONDS 1
#define BUFFER_SIZE 65000
#define SERVER_IP_ADDRESS "127.0.0.1"

#define MAX_RETRIES 5 // Maximum number of retries for sending packets

#define MAX_DATA_SIZE 1024

typedef struct {
    uint16_t length;
    uint16_t checksum;
    uint16_t seq_num; // Add sequence number field
    uint8_t flags;
    char data[MAX_DATA_SIZE];
} RUDP_Packet;



// Allocates a new structure for the RUDP socket (contains basic information about the socket itself). Also creates a UDP socket as a baseline for the RUDP.
// isServer means that this socket acts like a server. If set to server socket, it also binds the socket to a specific port.
int rudp_socket();

// Receives data from the other side and put it into the buffer. Returns the number of received bytes on success, 0 if got FIN packet (disconnect), and -1 on error.
// Fails if called when the socket is disconnected.
int rudp_recv(int sockfd, void *buf, size_t len, int flags, int port);

// Sends data stores in buffer to the other side. Returns the number of sent bytes on success, 0 if got FIN packet (disconnect), and -1 on error. Fails if called when the socket is disconnected.
int rudp_send(int sockfd, const void *buf, short len, int flags, const char *ip, int port);
// Disconnects from an actively connected socket. Returns 1 on success, 0 when the socket is already disconnected (failure).
//int rudp_disconnect(RUDP_Socket *sockfd);

// This function releases all the memory allocation and resources of the socket.
int rudp_close(int sockfd);

//this function calculates the checksum
unsigned short int calculate_checksum(void *data, unsigned int bytes);

// this function sends acknowledgment
void send_ack(int sockfd, struct sockaddr_in dest_addr, uint16_t seq_num);


