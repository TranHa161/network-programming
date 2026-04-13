#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#define PORT 8888
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// Trạng thái của client
typedef struct {
    int fd;
    int authenticated; // 0: chưa đăng nhập, 1: đã đăng nhập
} ClientState;

int check_login(char *user, char *pass) {
    FILE *f = fopen("users.txt", "r");
    if (f == NULL) return 0;

    char fuser[50], fpass[50];
    while (fscanf(f, "%s %s", fuser, fpass) != EOF) {
        if (strcmp(user, fuser) == 0 && strcmp(pass, fpass) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

int main() {
    int master_socket, new_socket, addrlen, max_sd, sd;
    struct sockaddr_in address;
    ClientState clients[MAX_CLIENTS];
    fd_set readfds;
    char buffer[BUFFER_SIZE];

    // Khởi tạo mảng client
    for (int i = 0; i < MAX_CLIENTS; i++) clients[i].fd = 0;

    // Tạo socket
    master_socket = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(master_socket, (struct sockaddr *)&address, sizeof(address));
    listen(master_socket, 3);

    printf("Server đang lắng nghe trên cổng %d...\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = clients[i].fd;
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        // Chấp nhận kết nối mới
        if (FD_ISSET(master_socket, &readfds)) {
            new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            printf("Kết nối mới: FD %d\n", new_socket);
            
            char *msg = "Chào mừng! Hãy nhập user và pass (định dạng: user pass):\n";
            send(new_socket, msg, strlen(msg), 0);

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == 0) {
                    clients[i].fd = new_socket;
                    clients[i].authenticated = 0;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = clients[i].fd;
            if (FD_ISSET(sd, &readfds)) {
                int valread = read(sd, buffer, BUFFER_SIZE);
                if (valread == 0) {
                    printf("Client ngắt kết nối: FD %d\n", sd);
                    close(sd);
                    clients[i].fd = 0;
                } else {
                    buffer[valread] = '\0';
                    strtok(buffer, "\r\n");

                    if (clients[i].authenticated == 0) {
                        char user[50], pass[50];
                        if (sscanf(buffer, "%s %s", user, pass) == 2 && check_login(user, pass)) {
                            clients[i].authenticated = 1;
                            char *ok = "Đăng nhập thành công! Nhập lệnh để thực thi:\n";
                            send(sd, ok, strlen(ok), 0);
                        } else {
                            char *err = "Sai tài khoản. Thử lại (user pass):\n";
                            send(sd, err, strlen(err), 0);
                        }
                    } else {
                        char cmd[BUFFER_SIZE + 20];
                        sprintf(cmd, "%s > out.txt 2>&1", buffer);
                        system(cmd);

                        // Đọc file out.txt gửi lại cho client
                        FILE *f = fopen("out.txt", "r");
                        if (f) {
                            while (fgets(buffer, BUFFER_SIZE, f) != NULL) {
                                send(sd, buffer, strlen(buffer), 0);
                            }
                            fclose(f);
                        }
                        send(sd, "\nDone.\n", 7, 0);
                    }
                }
            }
        }
    }
    return 0;
}