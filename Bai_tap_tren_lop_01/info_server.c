#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server, client;
    socklen_t len = sizeof(client);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&server, sizeof(server));
    listen(server_fd, 5);

    int sock = accept(server_fd, (struct sockaddr*)&client, &len);

    // Nhận path
    int path_len;
    recv(sock, &path_len, sizeof(int), 0);

    char path[1024];
    recv(sock, path, path_len, 0);
    path[path_len] = '\0';

    printf("%s\n", path);

    // Nhận số file
    int file_count;
    recv(sock, &file_count, sizeof(int), 0);

    for (int i = 0; i < file_count; i++) {
        int name_len;
        recv(sock, &name_len, sizeof(int), 0);

        char name[256];
        recv(sock, name, name_len, 0);
        name[name_len] = '\0';

        long size;
        recv(sock, &size, sizeof(long), 0);

        printf("%s - %ld bytes\n", name, size);
    }

    close(sock);
    close(server_fd);

    return 0;
}