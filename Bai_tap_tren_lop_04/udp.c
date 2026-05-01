#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Su dung: %s <port> <remote_ip> <remote_port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    char *remote_ip = argv[2];
    int remote_port = atoi(argv[3]);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in local_addr = {AF_INET, htons(port), INADDR_ANY};
    bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr));

    struct sockaddr_in remote_addr = {AF_INET, htons(remote_port)};
    inet_pton(AF_INET, remote_ip, &remote_addr.sin_addr);

    printf("Nhap tin nhan. Nhan Ctrl+C de thoat.\n");

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sock, &readfds);

        int max_fd = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;
        select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char buffer[BUFFER_SIZE];
            fgets(buffer, BUFFER_SIZE, stdin);
            sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
        }

        if (FD_ISSET(sock, &readfds)) {
            char buffer[BUFFER_SIZE];
            int n = recvfrom(sock, buffer, BUFFER_SIZE, 0, NULL, NULL);
            buffer[n] = '\0';
            printf("Received: %s", buffer);
        }
    }
    close(sock);
    return 0;
}