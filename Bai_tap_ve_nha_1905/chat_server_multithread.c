#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>

#define PORT 9000
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int sock;
    char id[50];
    int registered;
} Client;

Client clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast(int sender, char *msg) {
    pthread_mutex_lock(&clients_mutex);
    for(int i = 0; i < MAX_CLIENTS; i++) {
        if(clients[i].sock != 0 && clients[i].sock != sender && clients[i].registered) {
            send(clients[i].sock, msg, strlen(msg), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void get_time(char *buffer) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buffer, 64, "%Y/%m/%d %I:%M:%S%p", tm);
}

void *client_handler(void *arg) {
    int sd = *(int *)arg;
    free(arg);
    
    char buffer[BUFFER_SIZE];
    int client_index = -1;

    pthread_mutex_lock(&clients_mutex);
    for(int i = 0; i < MAX_CLIENTS; i++) {
        if(clients[i].sock == 0) {
            clients[i].sock = sd;
            clients[i].registered = 0;
            client_index = i;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    if(client_index == -1) {
        char *full_msg = "Server full. Please try again later.\n";
        send(sd, full_msg, strlen(full_msg), 0);
        close(sd);
        return NULL;
    }

    char *msg = "Enter id with format client_id: client_name\n";
    send(sd, msg, strlen(msg), 0);

    while(1) {
        int valread = recv(sd, buffer, BUFFER_SIZE - 1, 0);

        if(valread <= 0) {
            printf("Client disconnected\n");
            break;
        }

        buffer[valread] = 0;

        if(!clients[client_index].registered) {
            char id[50], name[50];

            if(sscanf(buffer, "%[^:]: %s", id, name) == 2) {
                pthread_mutex_lock(&clients_mutex);
                strcpy(clients[client_index].id, id);
                clients[client_index].registered = 1;
                pthread_mutex_unlock(&clients_mutex);

                char welcome_msg[100];
                sprintf(welcome_msg, "Welcome %s\n", id);
                send(sd, welcome_msg, strlen(welcome_msg), 0);

                printf("Client registered: %s\n", id);
            }
            else {
                char *err = "Wrong format. Use client_id: client_name\n";
                send(sd, err, strlen(err), 0);
            }
        }
        else {
            char timebuf[64];
            get_time(timebuf);

            char msg_buf[BUFFER_SIZE + 128];
            sprintf(msg_buf, "%s %s: %s", timebuf, clients[client_index].id, buffer);

            broadcast(sd, msg_buf);
            printf("%s", msg_buf);
        }
    }

    close(sd);
    pthread_mutex_lock(&clients_mutex);
    clients[client_index].sock = 0;
    clients[client_index].registered = 0;
    memset(clients[client_index].id, 0, sizeof(clients[client_index].id));
    pthread_mutex_unlock(&clients_mutex);

    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    for(int i = 0; i < MAX_CLIENTS; i++)
        clients[i].sock = 0;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    printf("Chat server started on port %d\n", PORT);

    while(1) {
        new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if(new_socket < 0) {
            perror("Accept error");
            continue;
        }

        printf("New connection\n");

        int *client_sock = malloc(sizeof(int));
        *client_sock = new_socket;

        pthread_t t;
        if(pthread_create(&t, NULL, client_handler, client_sock) != 0) {
            perror("Could not create thread");
            free(client_sock);
            close(new_socket);
        }
        
        pthread_detach(t);
    }

    close(server_fd);
    return 0;
}