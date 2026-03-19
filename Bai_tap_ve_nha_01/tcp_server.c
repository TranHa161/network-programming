#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    // 1. Kiểm tra tham số (Cổng, File chào, File log)
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <Port> <Greeting File> <Log File>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    char *greeting_file = argv[2];
    char *log_file = argv[3];

    // 2. Tạo socket TCP cho server
    int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // 3. Thiết lập địa chỉ để bind (Chấp nhận mọi giao diện mạng)
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    server_addr.sin_port = htons(port);

    // 4. Gắn socket với cổng xác định
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind() failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // 5. Lắng nghe kết nối (hàng đợi tối đa 5 kết nối)
    if (listen(server_sock, 5) < 0) {
        perror("listen() failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", port);

    // 6. Chấp nhận kết nối từ client
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
    if (client_sock < 0) {
        perror("accept() failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // 7. Đọc xâu chào từ file và gửi cho client
    FILE *f_greet = fopen(greeting_file, "r");
    if (f_greet != NULL) {
        char greet_msg[BUFFER_SIZE];
        if (fgets(greet_msg, BUFFER_SIZE, f_greet) != NULL) {
            send(client_sock, greet_msg, strlen(greet_msg), 0);
            printf("Greeting sent to client.\n");
        }
        fclose(f_greet);
    } else {
        perror("Warning: Greeting file not found");
    }

    // 8. Nhận dữ liệu từ client và ghi vào file log (chế độ append - ghi tiếp)
    FILE *f_log = fopen(log_file, "a");
    if (f_log == NULL) {
        perror("Could not open log file");
        close(client_sock);
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    int bytes_received;
    printf("Receiving data...\n");

    while ((bytes_received = recv(client_sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        fprintf(f_log, "%s", buffer);
        fflush(f_log); // Đẩy dữ liệu xuống đĩa ngay lập tức
    }

    // 9. Đóng các file và socket
    printf("Client disconnected. All data logged.\n");
    fclose(f_log);
    close(client_sock);
    close(server_sock);

    return 0;
}