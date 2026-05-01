#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

void encrypt(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        char c = str[i];
        if (c >= 'a' && c <= 'z') {
            str[i] = (c == 'z') ? 'a' : c + 1;
        } else if (c >= 'A' && c <= 'Z') {
            str[i] = (c == 'Z') ? 'A' : c + 1;
        } else if (c >= '0' && c <= '9') {
            str[i] = '0' + (9 - (c - '0'));
        }
    }
}

int main() {
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = {AF_INET, htons(8888), INADDR_ANY};
    
    bind(listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(listen_sock, 10);

    fd_set readfds;
    int client_socks[MAX_CLIENTS] = {0};
    int num_clients = 0;

    printf("Server started on port 8888...\n");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(listen_sock, &readfds);
        int max_fd = listen_sock;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_socks[i] > 0) {
                FD_SET(client_socks[i], &readfds);
                if (client_socks[i] > max_fd) max_fd = client_socks[i];
            }
        }

        select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(listen_sock, &readfds)) {
            int new_sock = accept(listen_sock, NULL, NULL);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_socks[i] == 0) {
                    client_socks[i] = new_sock;
                    num_clients++;
                    char msg[100];
                    sprintf(msg, "Xin chao! Hien co %d clients đang ket noi.\n", num_clients);
                    send(new_sock, msg, strlen(msg), 0);
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_socks[i] > 0 && FD_ISSET(client_socks[i], &readfds)) {
                char buffer[BUFFER_SIZE];
                int valread = recv(client_socks[i], buffer, BUFFER_SIZE, 0);
                if (valread <= 0) {
                    close(client_socks[i]);
                    client_socks[i] = 0;
                    num_clients--;
                } else {
                    buffer[valread] = '\0';
                    if (strncmp(buffer, "exit", 4) == 0) {
                        send(client_socks[i], "Tam biet\n", 9, 0);
                        close(client_socks[i]);
                        client_socks[i] = 0;
                        num_clients--;
                    } else {
                        encrypt(buffer);
                        send(client_socks[i], buffer, valread, 0);
                    }
                }
            }
        }
    }
    return 0;
}