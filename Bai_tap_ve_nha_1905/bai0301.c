#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <signal.h>

#define PORT 8080
#define STORAGE_DIR "./server_storage"

void handle_client(int client_sock) {
    DIR *d;
    struct dirent *dir;
    struct stat st;
    char buffer[1024];
    char file_list[4096] = "";
    char full_path[2048];
    int file_count = 0;

    d = opendir(STORAGE_DIR);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            snprintf(full_path, sizeof(full_path), "%s/%s", STORAGE_DIR, dir->d_name);

            if (stat(full_path, &st) == 0) {
                if (S_ISREG(st.st_mode)) {
                    strcat(file_list, dir->d_name);
                    strcat(file_list, "\r\n");
                    file_count++;
                }
            }
        }
        closedir(d);
    }

    if (file_count == 0) {
        send(client_sock, "ERROR No files to download \r\n", 29, 0);
        close(client_sock);
        exit(0);
    }

    sprintf(buffer, "OK %d\r\n", file_count);
    send(client_sock, buffer, strlen(buffer), 0);
    send(client_sock, file_list, strlen(file_list), 0);
    send(client_sock, "\r\n", 2, 0);
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) break;

        buffer[strcspn(buffer, "\r\n")] = 0;
        char filepath[2048];
        snprintf(filepath, sizeof(filepath), "%s/%s", STORAGE_DIR, buffer);

        struct stat st;
        if (stat(filepath, &st) == 0 && S_ISREG(st.st_mode)) {
            sprintf(buffer, "OK %ld\r\n", st.st_size);
            send(client_sock, buffer, strlen(buffer), 0);

            FILE *f = fopen(filepath, "rb");
            while ((bytes_received = fread(buffer, 1, sizeof(buffer), f)) > 0) {
                send(client_sock, buffer, bytes_received, 0);
            }
            fclose(f);
            break;
        } else {
            char *err_msg = "ERROR File not found. Please send again:\r\n";
            send(client_sock, err_msg, strlen(err_msg), 0);
        }
    }

    close(client_sock);
    exit(0);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);

    signal(SIGCHLD, SIG_IGN);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 10);

    printf("Server đang chạy tại port %d...\n", PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
        if (fork() == 0) {
            close(server_sock);
            handle_client(client_sock);
        } else {
            close(client_sock);
        }
    }

    return 0;
}