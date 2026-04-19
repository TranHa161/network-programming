#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    char id[50];
    int registered;
} Client;

Client clients[MAX_CLIENTS];

void get_time(char *buffer) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, 64, "%Y/%m/%d %H:%M:%S", t);
}

int main() {
    setenv("TZ", "Asia/Ho_Chi_Minh", 1);
    tzset();
    int server_fd, new_fd;
    struct sockaddr_in addr;
    struct pollfd fds[MAX_CLIENTS + 1];

    char buffer[BUFFER_SIZE];

    // Tạo socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    int nfds = 1;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
    }

    printf("Server running on port %d...\n", PORT);

    while (1) {
        poll(fds, nfds, -1);

        // Có client mới
        if (fds[0].revents & POLLIN) {
            new_fd = accept(server_fd, NULL, NULL);

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == -1) {
                    clients[i].fd = new_fd;
                    clients[i].registered = 0;

                    fds[nfds].fd = new_fd;
                    fds[nfds].events = POLLIN;
                    nfds++;

                    send(new_fd, "Nhap ten (client_id: name):\n", 32, 0);
                    break;
                }
            }
        }

        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                int fd = fds[i].fd;

                int n = recv(fd, buffer, BUFFER_SIZE - 1, 0);

                if (n <= 0) {
                    close(fd);
                    fds[i] = fds[nfds - 1];
                    nfds--;
                    i--;
                    continue;
                }

                buffer[n] = '\0';

                int idx = -1;
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (clients[j].fd == fd) {
                        idx = j;
                        break;
                    }
                }

                if (idx == -1) continue;

                if (!clients[idx].registered) {
                    char id[50], name[50];

                    if (sscanf(buffer, "%[^:]: %s", id, name) == 2) {
                        strcpy(clients[idx].id, id);
                        clients[idx].registered = 1;

                        send(fd, "Dang ky thanh cong!\n", 21, 0);
                    } else {
                        send(fd, "Sai cu phap! Nhap lai:\n", 25, 0);
                    }
                } 
                else {
                    char timebuf[64];
                    get_time(timebuf);

                    char msg[BUFFER_SIZE + 100];
                    snprintf(msg, sizeof(msg), "%s %s: %.900s", timebuf, clients[idx].id, buffer);

                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j].fd != -1 && clients[j].fd != fd) {
                            send(clients[j].fd, msg, strlen(msg), 0);
                        }
                    }
                }
            }
        }
    }

    return 0;
}