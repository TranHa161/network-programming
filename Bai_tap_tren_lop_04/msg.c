#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/select.h>

#define PORT 9000
#define MAX_CLIENTS 30
#define MAX_TOPICS 10

typedef struct {
    int fd;
    char topics[MAX_TOPICS][32];
    int topic_count;
} Client;

Client clients[MAX_CLIENTS] = {0};

void send_to_topic(char *topic, char *msg, int sender_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != 0) {
            for (int j = 0; j < clients[i].topic_count; j++) {
                if (strcmp(clients[i].topics[j], topic) == 0) {
                    send(clients[i].fd, msg, strlen(msg), 0);
                    send(clients[i].fd, "\n", 1, 0);
                }
            }
        }
    }
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(PORT), INADDR_ANY};
    
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);
    printf("Server dang chay tai cong %d...\n", PORT);

    fd_set readfds;
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd > 0) {
                FD_SET(clients[i].fd, &readfds);
                if (clients[i].fd > max_sd) max_sd = clients[i].fd;
            }
        }

        select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &readfds)) {
            int new_sd = accept(server_fd, NULL, NULL);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == 0) {
                    clients[i].fd = new_sd;
                    clients[i].topic_count = 0;
                    break;
                }
            }
        } else {
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd > 0 && FD_ISSET(clients[i].fd, &readfds)) {
                    char buffer[1024];
                    int valread = read(clients[i].fd, buffer, 1024);
                    if (valread <= 0) { 
                        close(clients[i].fd);
                        clients[i].fd = 0; 
                    } else {
                        buffer[valread] = '\0';
                        char cmd[10], topic[32], msg[1024];
                        sscanf(buffer, "%s %s %[^\n]", cmd, topic, msg);

                        if (strcmp(cmd, "SUB") == 0) {
                            if (clients[i].topic_count < MAX_TOPICS)
                                strcpy(clients[i].topics[clients[i].topic_count++], topic);
                        } else if (strcmp(cmd, "UNSUB") == 0) {
                            for (int j = 0; j < clients[i].topic_count; j++) {
                                if (strcmp(clients[i].topics[j], topic) == 0) {
                                    strcpy(clients[i].topics[j], clients[i].topics[clients[i].topic_count - 1]);
                                    clients[i].topic_count--;
                                    break;
                                }
                            }
                        } else if (strcmp(cmd, "PUB") == 0) {
                            send_to_topic(topic, msg, clients[i].fd);
                        }
                    }
                }
            }
        }
    }
    return 0;
}