#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define BUFFER_SIZE 1024

void *time_handler(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);
    
    char buf[BUFFER_SIZE];
    char welcome[] = "Nhap lenh theo thong tin: GET_TIME [format]\nCac format: dd/mm/yyyy, dd/mm/yy, mm/dd/yyyy, mm/dd/yy\n";
    send(client_socket, welcome, strlen(welcome), 0);

    while (1) {
        int ret = recv(client_socket, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) break;
        buf[ret] = '\0';
        buf[strcspn(buf, "\r\n")] = 0;

        char cmd[20], format[50];
        int fields = sscanf(buf, "%s %s", cmd, format);

        if (fields == 2 && strcmp(cmd, "GET_TIME") == 0) {
            time_t rawtime;
            struct tm *timeinfo;
            time(&rawtime);
            timeinfo = localtime(&rawtime);

            char time_out[100];
            int valid = 1;

            if (strcmp(format, "dd/mm/yyyy") == 0) {
                strftime(time_out, sizeof(time_out), "%d/%m/%Y\n", timeinfo);
            } else if (strcmp(format, "dd/mm/yy") == 0) {
                strftime(time_out, sizeof(time_out), "%d/%m/%y\n", timeinfo);
            } else if (strcmp(format, "mm/dd/yyyy") == 0) {
                strftime(time_out, sizeof(time_out), "%m/%d/%Y\n", timeinfo);
            } else if (strcmp(format, "mm/dd/yy") == 0) {
                strftime(time_out, sizeof(time_out), "%m/%d/%y\n", timeinfo);
            } else {
                valid = 0;
            }

            if (valid) {
                send(client_socket, time_out, strlen(time_out), 0);
            } else {
                char err[] = "Sai dinh dang format thời gian!\n";
                send(client_socket, err, strlen(err), 0);
            }
        } else {
            char err_cmd[] = "Sai cu phap lenh! Phai bat dau bang GET_TIME\n";
            send(client_socket, err_cmd, strlen(err_cmd), 0);
        }
    }

    close(client_socket);
    return NULL;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(7777), INADDR_ANY};
    
    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 10);
    printf("Time Server đang chạy tại port 7777...\n");

    while (1) {
        int *client_sock = malloc(sizeof(int));
        *client_sock = accept(server_fd, NULL, NULL);
        pthread_t t;
        pthread_create(&t, NULL, time_handler, client_sock);
        pthread_detach(t);
    }
    close(server_fd);
    return 0;
}