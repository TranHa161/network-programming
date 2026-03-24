#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUF_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server;
    char buf[BUF_SIZE];

    // Tạo socket
    sock = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    // Kết nối server
    connect(sock, (struct sockaddr*)&server, sizeof(server));

    printf("Da ket noi server!\n");

    while (1) {
        printf("Nhap chuoi (exit de thoat): "); 
        fflush(stdout);

        if (fgets(buf, BUF_SIZE, stdin) == NULL) break;

        // Xóa ký tự xuống dòng \n ở cuối (nếu có)
        buf[strcspn(buf, "\r\n")] = 0;

        // Nếu người dùng gõ exit
        if (strcmp(buf, "exit") == 0) break;

        if (strlen(buf) == 0) continue;

        send(sock, buf, strlen(buf), 0);
    }

    close(sock);
    return 0;
}