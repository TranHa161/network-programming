#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUF_SIZE 2048
#define PATTERN "0123456789"
#define PAT_LEN 10

int main() {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    
    char storage[BUF_SIZE * 2]; // Buffer lưu dữ liệu
    int storage_len = 0;
    int total_count = 0;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 1);

    printf("Server dang chay...\n");
    client_fd = accept(server_fd, NULL, NULL);
    printf("Client da ket noi!\n");

    char recv_buf[BUF_SIZE];
    while (1) {
        int n = recv(client_fd, recv_buf, BUF_SIZE, 0);
        if (n <= 0) break;

        memcpy(storage + storage_len, recv_buf, n);
        storage_len += n;

        for (int i = 0; i <= storage_len - PAT_LEN; i++) {
            if (memcmp(storage + i, PATTERN, PAT_LEN) == 0) {
                total_count++;
            }
        }

        if (storage_len >= PAT_LEN) {
            int keep = PAT_LEN - 1;
            memmove(storage, storage + storage_len - keep, keep);
            storage_len = keep;
        }

        printf("Tong so lan xuat hien: %d\n", total_count);
    }

    close(client_fd);
    close(server_fd);
    return 0;
}