#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

// Struct phải giống hệt phía Client
typedef struct {
    char mssv[15];
    char name[50];
    char birthday[15];
    float gpa;
} Student;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Port> <Log File>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));

    bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);
    printf("Server is waiting for student data on port %s...\n", argv[1]);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);

        if (client_sock < 0) {
            perror("accept() failed");
            continue;
        }

        char *client_ip = inet_ntoa(client_addr.sin_addr);
        printf("Accepted connection from %s\n", client_ip);

        Student sv;
        // VÒNG LẶP MỚI: Nhận liên tục từ Client này cho đến khi họ ngắt kết nối (recv trả về 0)
        while (recv(client_sock, &sv, sizeof(sv), 0) > 0) {
            
            // 1. Lấy thời gian hiện tại
            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            char time_str[20];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

            // 2. Ghi vào file log
            FILE *f_log = fopen(argv[2], "a");
            if (f_log) {
                fprintf(f_log, "%s %s %s %s %s %.2f\n", 
                        client_ip, time_str, sv.mssv, sv.name, sv.birthday, sv.gpa);
                fclose(f_log);
                printf("%s %s %s %s %s %.2f\n", 
                        client_ip, time_str, sv.mssv, sv.name, sv.birthday, sv.gpa);
            }
        }

        printf("Client %s disconnected.\n", client_ip);
        close(client_sock);
    }

    close(server_sock);
    return 0;
}