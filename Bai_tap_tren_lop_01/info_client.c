#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server.sin_addr); 

    connect(sock, (struct sockaddr*)&server, sizeof(server));

    // Lấy thư mục hiện tại
    char cwd[1024];
    getcwd(cwd, sizeof(cwd)); // Lấy đường dẫn thư mục hiện tại
    int path_len = strlen(cwd);

    send(sock, &path_len, sizeof(int), 0); // Gửi độ dài chuỗi
    send(sock, cwd, path_len, 0); // Gửi nội dung chuỗi

    // Đọc file
    DIR *dir = opendir("."); // Mở thư mục hiện tại
    struct dirent *entry;

    int file_count = 0;

    // Đếm trước
    while ((entry = readdir(dir)) != NULL) {
        struct stat st;
        // Kiểm tra nếu là file thông thường
        if (stat(entry->d_name, &st) == 0 && S_ISREG(st.st_mode)) {
            file_count++;
        }
    }
    rewinddir(dir); // Đưa con trỏ thư mục về lại đầu để đọc lần 2

    send(sock, &file_count, sizeof(int), 0);

    // Gửi từng file
    while ((entry = readdir(dir)) != NULL) {
        struct stat st;

        if (stat(entry->d_name, &st) == 0 && S_ISREG(st.st_mode)) {

            int name_len = strlen(entry->d_name);

            send(sock, &name_len, sizeof(int), 0);
            send(sock, entry->d_name, name_len, 0);
            send(sock, &st.st_size, sizeof(long), 0);
        }
    }

    closedir(dir);
    close(sock);

    return 0;
}