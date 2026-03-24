#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 8080
#define BUF_SIZE 1024

int sockfd;

// Xử lý Ctrl+C
void handle_sigint(int sig) {
    printf("\nServer dang tat...\n");
    close(sockfd);
    _exit(0);
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t len;

    char buf[BUF_SIZE];

    // Bắt Ctrl+C
    signal(SIGINT, handle_sigint);

    // Tạo socket UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return 1;
    }

    printf("UDP Server dang chay...\n");

    while (1) {
        len = sizeof(client_addr); // reset mỗi lần

        int n = recvfrom(sockfd, buf, BUF_SIZE - 1, 0,
                         (struct sockaddr*)&client_addr, &len);

        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        buf[n] = '\0';

        printf("Nhan duoc: %s\n", buf);

        // Echo lại
        sendto(sockfd, buf, n, 0,
               (struct sockaddr*)&client_addr, len);
    }

    return 0;
}