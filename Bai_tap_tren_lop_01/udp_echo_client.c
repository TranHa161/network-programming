#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 8080
#define BUF_SIZE 1024

int sockfd;

// Hàm xử lý Ctrl+C
void handle_sigint(int sig) {
    printf("\nDang thoat...\n");
    close(sockfd);
    _exit(0);
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t len = sizeof(server_addr);

    char buf[BUF_SIZE];

    // Bắt Ctrl+C
    signal(SIGINT, handle_sigint);

    // Tạo socket UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    printf("Nhap chuoi (Ctrl+C de thoat):\n");

    while (1) {
        printf("> ");
        fgets(buf, BUF_SIZE, stdin);

        buf[strcspn(buf, "\n")] = 0;

        // Gửi dữ liệu
        sendto(sockfd, buf, strlen(buf), 0,
               (struct sockaddr*)&server_addr, len);

        // Nhận echo
        int n = recvfrom(sockfd, buf, BUF_SIZE - 1, 0, NULL, NULL);
        buf[n] = '\0';

        printf("Server tra ve: %s\n", buf);
    }

    close(sockfd);
    return 0;
}