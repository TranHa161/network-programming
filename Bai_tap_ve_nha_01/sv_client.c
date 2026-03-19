#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

// Định nghĩa cấu trúc sinh viên để gửi đi
typedef struct {
    char mssv[15];
    char name[50];
    char birthday[15];
    float gpa;
} Student;

int main(int argc, char *argv[]) {
    // 1. Kiểm tra tham số dòng lệnh
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP Address> <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server. Enter student info (Press Ctrl+C to stop):\n");

    Student sv;
    while (1) {
        printf("\nEnter new student\n");
        
        // 1. Nhập MSSV
        printf("MSSV: "); 
        if (scanf("%s", sv.mssv) <= 0) break; 
        
        // 2. Nhập Họ tên
        getchar(); 
        printf("Full Name: "); 
        fgets(sv.name, sizeof(sv.name), stdin);
        sv.name[strcspn(sv.name, "\n")] = 0; // Xóa ký tự \n ở cuối chuỗi

        // 3. Nhập Ngày sinh
        printf("Birthday (YYYY-MM-DD): "); 
        scanf("%s", sv.birthday);

        // 4. Nhập Điểm GPA
        printf("GPA: "); 
        scanf("%f", &sv.gpa);

        // 5. Gửi cấu trúc sang Server
        if (send(client_sock, &sv, sizeof(sv), 0) < 0) {
            perror("Send failed");
            break;
        }
        printf("Data sent successfully!\n");
    }

    close(client_sock);
    return 0;
}