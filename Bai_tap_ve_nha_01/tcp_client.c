#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    // 1. Kiểm tra tham số dòng lệnh (Yêu cầu: <IP> <Port>)
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP Address> <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    // 2. Tạo socket TCP
    int client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_sock < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // 3. Thiết lập cấu trúc địa chỉ server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); 
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    // Chuyển đổi địa chỉ IP từ dạng văn bản sang dạng nhị phân
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid IP address format!\n");
        close(client_sock);
        exit(EXIT_FAILURE);
    }

    // 4. Thực hiện kết nối tới server
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect() failed");
        close(client_sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to %s:%d\n", ip, port);
    // Nhận xâu chào từ Server
    char greeting[BUFFER_SIZE];
    int n = recv(client_sock, greeting, sizeof(greeting) - 1, 0);
    if (n > 0) {
        greeting[n] = '\0';
        printf("\n%s", greeting);
    }
    printf("Enter message (Press Ctrl+D to end):\n");

    // 5. Vòng lặp nhận dữ liệu từ bàn phím và gửi đi
    char buffer[BUFFER_SIZE];
    while (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
        if (send(client_sock, buffer, strlen(buffer), 0) == -1) {
            perror("send() failed");
            break;
        }
    }

    // 6. Đóng socket và kết thúc
    close(client_sock);
    printf("Disconnected.\n");
    return 0;
}