#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

#define PORT 8888
#define MAX_CLIENTS 10

typedef struct {
    int fd;
    int authenticated;
} Client;

Client clients[MAX_CLIENTS];

int check_auth(char *user, char *pass) {
    FILE *f = fopen("users.txt", "r");
    if (!f) return 0;

    char u[50], p[50];
    while (fscanf(f, "%s %s", u, p) != EOF) {
        if (strcmp(u, user) == 0 && strcmp(p, pass) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

int find_client(int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == fd) return i;
    }
    return -1;
}

void remove_client(struct pollfd fds[], int *nfds, int i) {
    close(fds[i].fd);

    int idx = find_client(fds[i].fd);
    if (idx != -1) {
        clients[idx].fd = -1;
        clients[idx].authenticated = 0;
    }

    fds[i] = fds[*nfds - 1];
    (*nfds)--;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 5);

    struct pollfd fds[MAX_CLIENTS + 1];
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    int nfds = 1;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
    }

    while (1) {
        poll(fds, nfds, -1);

        if (fds[0].revents & POLLIN) {
            int client_fd = accept(server_fd, NULL, NULL);

            if (nfds < MAX_CLIENTS + 1) {
                fds[nfds].fd = client_fd;
                fds[nfds].events = POLLIN;
                nfds++;

                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].fd == -1) {
                        clients[i].fd = client_fd;
                        clients[i].authenticated = 0;
                        break;
                    }
                }

                send(client_fd, "Enter user pass: ", 18, 0);
            } else {
                close(client_fd);
            }
        }

        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                char buffer[256];
                int bytes = recv(fds[i].fd, buffer, sizeof(buffer)-1, 0);

                if (bytes <= 0) {
                    remove_client(fds, &nfds, i);
                    i--;
                    continue;
                }

                buffer[bytes] = '\0';

                int idx = find_client(fds[i].fd);
                if (idx == -1) continue;

                if (!clients[idx].authenticated) {
                    char user[50], pass[50];

                    if (sscanf(buffer, "%s %s", user, pass) == 2) {
                        if (check_auth(user, pass)) {
                            clients[idx].authenticated = 1;
                            send(fds[i].fd, "Login success!\nEnter command:\n", 32, 0);
                        } else {
                            send(fds[i].fd, "Login failed!\n", 14, 0);
                        }
                    } else {
                        send(fds[i].fd, "Format: user pass\n", 19, 0);
                    }
                } else {
                    buffer[strcspn(buffer, "\r\n")] = 0;

                    char cmd[300];
                    snprintf(cmd, sizeof(cmd), "%s > out.txt", buffer);
                    system(cmd);

                    FILE *f = fopen("out.txt", "r");
                    if (!f) {
                        send(fds[i].fd, "Error reading output\n", 21, 0);
                        continue;
                    }

                    char line[256];
                    while (fgets(line, sizeof(line), f)) {
                        send(fds[i].fd, line, strlen(line), 0);
                    }
                    fclose(f);
                }
            }
        }
    }
}