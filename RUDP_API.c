#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include "RUDP_API.h"



unsigned short int calculate_checksum(void *data, unsigned int bytes) {
    unsigned short int *data_pointer = (unsigned short int *)data;
    unsigned int total_sum = 0;

    // Main summing loop
    while (bytes > 1) {
        total_sum += *data_pointer++;
        bytes -= 2;
    }

    // Add left-over byte, if any
    if (bytes > 0)
        total_sum += *((unsigned char *)data_pointer);

    // Fold 32-bit sum to 16 bits
    while (total_sum >> 16)
        total_sum = (total_sum & 0xFFFF) + (total_sum >> 16);

    return (~((unsigned short int)total_sum));
}

int rudp_socket() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("Failed to create UDP socket");
        return -1;
    }
    // Additional initialization code for RUDP protocol parameters

    int buffer_size = 64 * 1024; // 64KB
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size)) < 0) {
        perror("Failed to set send buffer size");
        return -1;
    }

    return sockfd;
}

static int last_seq_sent = 0; // Variable to keep track of last sequence number sent

int rudp_send(int sockfd, const void *buf, short len, int flags, const char *ip, int port) {
    RUDP_Packet *packet = malloc(sizeof(RUDP_Packet));
    if (packet == NULL) {
        perror("Failed to allocate memory for packet");
        return -1;
    }
    // Increment sequence number for each packet
    packet->seq_num = last_seq_sent++;

    packet->length = (uint16_t)len;
    memcpy(packet->data, buf, len);
    packet->checksum = calculate_checksum(packet->data, packet->length);

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    dest_addr.sin_addr.s_addr = inet_addr(ip);

    // Code for sending packet and handling acknowledgment
    int retries = 0;
    while (retries < MAX_RETRIES) {
        // Send packet
        if (sendto(sockfd, packet, sizeof(RUDP_Packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
            perror("Failed to send packet");
            free(packet);
            return -1;
        }

        // Set timeout for receiving acknowledgment
        struct timeval tv;
        tv.tv_sec = TIMEOUT_SECONDS;
        tv.tv_usec = TIMEOUT_MICROSECS;

        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            perror("Error setting timeout for acknowledgment");
            free(packet);
            return -1;
        }

        // Wait for acknowledgment
        RUDP_Packet ack_packet;
        ssize_t recv_size = recvfrom(sockfd, &ack_packet, sizeof(RUDP_Packet), 0, NULL, NULL);
        if (recv_size > 0 && ack_packet.seq_num == packet->seq_num) {
            // Acknowledgment received for correct sequence number
            free(packet);
            return 0; // Success
        } else {
            // Acknowledgment not received or received for wrong sequence number, retry
            retries++;
        }

 

    // if (sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
    //     perror("Failed to send packet");
    //     return -1;
    // }

    perror("Maximum retries reached without acknowledgment");
    free(packet);
    return -1;
    }


}

// Global variable to store the next expected sequence number
static int next_expected_seq = 0;

void send_ack(int sockfd, struct sockaddr_in dest_addr, uint16_t seq_num) {
    // Create acknowledgment packet
    RUDP_Packet ack_packet;
    memset(&ack_packet, 0, sizeof(RUDP_Packet));
    ack_packet.seq_num = seq_num; // Acknowledge the specified sequence number

    // Send acknowledgment
    if (sendto(sockfd, &ack_packet, sizeof(RUDP_Packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("Failed to send acknowledgment");
    }
}

int rudp_recv(int sockfd, void *buf, size_t len, int flags, int port) {

    struct sockaddr_in from_addr;
    memset(&from_addr, 0, sizeof(from_addr));
    // from_addr.sin_family = AF_INET;
    // from_addr.sin_port = htons(port);
    // from_addr.sin_addr.s_addr = htons(INADDR_ANY);
    socklen_t from_len = sizeof(from_addr);
    // Code for receiving packet and sending acknowledgment
    RUDP_Packet *packet;
    ssize_t received = recvfrom(sockfd, &packet, sizeof(RUDP_Packet), flags, (struct sockaddr *)&from_addr, &from_len);
    if (received < 0) {
        perror("Failed to receive data");
        return -1;
    }

    // // Check sequence number of received packet
    // if (packet->seq_num == next_expected_seq) {
    //     // Packet received in order
    //     // Send acknowledgment
    //     send_ack(sockfd, from_addr, packet->seq_num);

    //     // Update next expected sequence number
    //     next_expected_seq++;

    //     // Copy data to user buffer if valid and packet has content
    //     size_t data_length = packet->length < len ? packet->length : len;  // Ensure we do not overflow the buffer
    //     memcpy(buf, packet->data, data_length);

    //     return data_length;  // Return the number of bytes copied to the buffer
    // }
    // else {
    //     // Packet received out of order, discard
    //     // Send acknowledgment for last correctly received packet
    //     send_ack(sockfd, from_addr, next_expected_seq - 1);
    //     return 0; // No data received
    // }

    if(packet->seq_num == next_expected_seq){
        send_ack(sockfd, from_addr, packet->seq_num);

        next_expected_seq++;

        size_t data_length = packet->length < len ? packet->length : len;
        memcpy(buf, packet->data, data_length);

        return data_length;
    }
    else{
        send_ack(sockfd, from_addr, next_expected_seq -1);
        return 0;
    }
}



// Global variable to store the next expected sequence number
// static int next_expected_seq = 0;

// int rudp_recv(int sockfd, void *buf, size_t len, int flags, int port) {
//     struct sockaddr_in from_addr;
//     memset(&from_addr, 0, sizeof(from_addr));
//     // from_addr.sin_family = AF_INET;
//     // from_addr.sin_port = htons(port);
//     // from_addr.sin_addr.s_addr = htons(INADDR_ANY);
//     socklen_t from_len = sizeof(from_addr);
    

//     // Initialize buffer to zero
//     memset(&packet, 0, sizeof(packet));
//     RUDP_Packet *packet = malloc(sizeof(RUDP_Packet));
//     // Receive a packet
//     ssize_t received = recvfrom(sockfd, &packet, sizeof(packet), flags, (struct sockaddr *)&from_addr, &from_len);
//     if (received < 0) {
//         perror("Failed to receive data");
//         return -1;
//     }

//     // Calculate checksum for the received packet
//     unsigned short original_checksum = packet.checksum;  // Store original checksum
//     // packet.checksum = 0;  // Zero out checksum for calculation
//     unsigned short calculated_checksum = calculate_checksum(&packet, sizeof(packet));

//     // Check checksum validity
//     if (original_checksum != calculated_checksum) {
//         fprintf(stderr, "Checksum mismatch: expected %hu, got %hu\n", calculated_checksum, original_checksum);
//         return -1;  // Return error for checksum mismatch
//     }

//     // Copy data to user buffer if valid and packet has content
//     size_t data_length = packet.length < len ? packet.length : len;  // Ensure we do not overflow the buffer
//     memcpy(buf, packet.data, data_length);

//     return data_length;  // Return the number of bytes copied to the buffer
// }

int rudp_close(int sockfd) {
	if(close(sockfd) == -1) {
		perror("Failed to close socket");
		return -1;
	}
	return 0;
}
