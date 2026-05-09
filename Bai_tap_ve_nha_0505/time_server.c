#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>

#define PORT 9999
#define BUFFER_SIZE 1024

void get_formatted_time(char *format, char *output) {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    if (strcmp(format, "dd/mm/yyyy") == 0) {
        strftime(output, 20, "%d/%m/%Y", timeinfo);
    } else if (strcmp(format, "dd/mm/yy") == 0) {
        strftime(output, 20, "%d/%m/%y", timeinfo);
    } else if (strcmp(format, "mm/dd/yyyy") == 0) {
        strftime(output, 20, "%m/%d/%Y", timeinfo);
    } else if (strcmp(format, "mm/dd/yy") == 0) {
        strftime(output, 20, "%m/%d/%y", timeinfo);
    } else {
        strcpy(output, "INVALID_FORMAT");
    }
}

void handle_client(int client_sock) {
    char buffer[BUFFER_SIZE];
    char cmd[20], format[50], res[50];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int n = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;

        buffer[n] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;

        int count = sscanf(buffer, "%s %s", cmd, format);

        if (count == 2 && strcmp(cmd, "GET_TIME") == 0) {
            get_formatted_time(format, res);
            if (strcmp(res, "INVALID_FORMAT") == 0) {
                char *msg = "Lỗi: Định dạng không được hỗ trợ.\n";
                send(client_sock, msg, strlen(msg), 0);
            } else {
                strcat(res, "\n");
                send(client_sock, res, strlen(res), 0);
            }
        } else {
            char *msg = "Lỗi: Sai cú pháp. Cú pháp đúng: GET_TIME [format]\n";
            send(client_sock, msg, strlen(msg), 0);
        }
    }

    close(client_sock);
    exit(0);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    listen(listener, 10);
    printf("Time Server đang chạy trên port %d...\n", PORT);

    signal(SIGCHLD, SIG_IGN);

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client < 0) continue;

        if (fork() == 0) {
            close(listener);
            handle_client(client);
        } else {
            close(client);
        }
    }

    return 0;
}