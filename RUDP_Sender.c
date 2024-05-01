#include "RUDP_API.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <PORT>\n", argv[0]);
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);
    char buffer[1024];
    FILE *file;

//    int sockfd = rudp_socket(AF_INET, SOCK_DGRAM, 0);
    int sockfd = rudp_socket();

    if (sockfd < 0) {
        perror("Failed to create UDP socket");
        return 1;
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    dest_addr.sin_addr.s_addr = inet_addr(ip);
    char decision[10];  // Buffer for user decision

    do {
        printf("Enter the filename to send: ");
       // scanf("%1023s", buffer);
        fgets(buffer, 1024, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        //getchar();  // Consume the newline character to prevent it from being read in the loop
        printf("file: %s\n", buffer);
        file = fopen(buffer, "rb");
        if (file == NULL) {
            perror("Failed to open file");
            continue;
        }

        while (!feof(file)) {
            size_t read_bytes = fread(buffer, 1, sizeof(buffer), file);
            if (ferror(file)) {
                perror("Read error");
                break;
            }
            if (read_bytes > 0) {
                if (rudp_send(sockfd, buffer, read_bytes, 0, ip, port) < 0) {
                    perror("Failed to send data");
                }
            }
        }

        fclose(file);

        printf("Send the file again? (yes/no): ");
        fgets(decision, sizeof(decision), stdin);
        decision[strcspn(decision, "\n")] = 0;  // Remove trailing newline

    } while (strncmp(decision, "yes", 3) == 0);

    strcpy(buffer, "exit");
    rudp_send(sockfd, buffer, strlen(buffer), 0, ip, port);
    rudp_close(sockfd);

    return 0;
}
