#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define PORT 8080

void *http_handler(void *arg) {
    int client = *(int *)arg;
    free(arg);

    printf("New client connected: %d\n", client);

    char buf[256];
    int ret = recv(client, buf, sizeof(buf) - 1, 0);
    
    if (ret > 0) {
        buf[ret] = 0;
        puts(buf);

        char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Xin chao cac ban</h1></body></html>";
        send(client, msg, strlen(msg), 0);
    }

    close(client);
    return NULL;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        close(listener);
        exit(EXIT_FAILURE);
    }

    if (listen(listener, 20) < 0) {
        perror("Listen failed");
        close(listener);
        exit(EXIT_FAILURE);
    }

    printf("HTTP Multithread Server dang chay tai port %d...\n", PORT);

    while (1) {
        int *client_sock = malloc(sizeof(int));
        *client_sock = accept(listener, NULL, NULL);
        
        if (*client_sock >= 0) {
            pthread_t t;
            if (pthread_create(&t, NULL, http_handler, client_sock) == 0) {
                pthread_detach(t);
            } else {
                perror("Thread creation failed");
                free(client_sock);
            }
        } else {
            free(client_sock);
        }
    }

    close(listener);
    return 0;
}