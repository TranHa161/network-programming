#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
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

void broadcast(int sender, char *msg) {
    for(int i=0;i<MAX_CLIENTS;i++) {
        if(clients[i].sock != 0 && clients[i].sock != sender) {
            send(clients[i].sock, msg, strlen(msg), 0);
        }
    }
}

void get_time(char *buffer) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buffer, 64, "%Y/%m/%d %I:%M:%S%p", tm);
}

int main() {

    int server_fd, new_socket, activity;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    fd_set readfds;

    char buffer[BUFFER_SIZE];

    for(int i=0;i<MAX_CLIENTS;i++)
        clients[i].sock = 0;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    printf("Chat server started on port %d\n", PORT);

    while(1) {

        FD_ZERO(&readfds);

        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        for(int i=0;i<MAX_CLIENTS;i++) {
            int sd = clients[i].sock;
            if(sd > 0)
                FD_SET(sd, &readfds);

            if(sd > max_sd)
                max_sd = sd;
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if(FD_ISSET(server_fd, &readfds)) {

            new_socket = accept(server_fd,
                        (struct sockaddr*)&address,
                        (socklen_t*)&addrlen);

            printf("New connection\n");

            for(int i=0;i<MAX_CLIENTS;i++) {
                if(clients[i].sock == 0) {
                    clients[i].sock = new_socket;
                    clients[i].registered = 0;
                    break;
                }
            }

            char *msg = "Enter id with format client_id: client_name\n";
            send(new_socket, msg, strlen(msg), 0);
        }

        for(int i=0;i<MAX_CLIENTS;i++) {

            int sd = clients[i].sock;

            if(FD_ISSET(sd, &readfds)) {

                int valread = recv(sd, buffer, BUFFER_SIZE, 0);

                if(valread == 0) {
                    close(sd);
                    clients[i].sock = 0;
                    printf("Client disconnected\n");
                }

                else {

                    buffer[valread] = 0;

                    if(!clients[i].registered) {

                        char id[50], name[50];

                        if(sscanf(buffer, "%[^:]: %s", id, name) == 2) {
                            strcpy(clients[i].id, id);
                            clients[i].registered = 1;

                            char msg[100];
                            sprintf(msg, "Welcome %s\n", id);
                            send(sd, msg, strlen(msg), 0);

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

                        char msg[BUFFER_SIZE];

                        sprintf(msg,"%s %s: %s",
                                timebuf,
                                clients[i].id,
                                buffer);

                        broadcast(sd, msg);

                        printf("%s", msg);
                    }
                }
            }
        }
    }

    return 0;
}