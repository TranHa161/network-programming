#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024

int check_login(const char *user, const char *pass) {
    FILE *f = fopen("accounts.txt", "r");
    if (!f) return 0;
    
    char u[50], p[50];
    while (fscanf(f, "%s %s", u, p) != EOF) {
        if (strcmp(user, u) == 0 && strcmp(pass, p) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

void send_file_content(int client_socket, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        char err[] = "Khong the mo file ket qua.\n";
        send(client_socket, err, strlen(err), 0);
        return;
    }
    char buf[BUFFER_SIZE];
    while (fgets(buf, sizeof(buf), f) != NULL) {
        send(client_socket, buf, strlen(buf), 0);
    }
    fclose(f);
}

void *telnet_handler(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);
    
    char buf[BUFFER_SIZE];
    char user[50], pass[50];
    int logged_in = 0;

    while (!logged_in) {
        char ask_msg[] = "Nhap user va pass (vi du: admin 12345):\n";
        send(client_socket, ask_msg, strlen(ask_msg), 0);
        
        int ret = recv(client_socket, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) { close(client_socket); return NULL; }
        buf[ret] = '\0';
        
        if (sscanf(buf, "%s %s", user, pass) == 2) {
            if (check_login(user, pass)) {
                char ok_msg[] = "Dang nhap thanh cong! Hay gui lenh (vd: dir hoac ls):\n";
                send(client_socket, ok_msg, strlen(ok_msg), 0);
                logged_in = 1;
            } else {
                char err_msg[] = "Loi dang nhap! Sai user hoac pass.\n";
                send(client_socket, err_msg, strlen(err_msg), 0);
            }
        }
    }

    while (1) {
        int ret = recv(client_socket, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) break;
        buf[ret] = '\0';
        buf[strcspn(buf, "\r\n")] = 0;

        if (strlen(buf) == 0) continue;

        char cmd[BUFFER_SIZE + 128];
        char out_filename[50];
        sprintf(out_filename, "out_%d.txt", client_socket);
        
        sprintf(cmd, "%s > %s 2>&1", buf, out_filename); 
        system(cmd);

        send_file_content(client_socket, out_filename);
        send(client_socket, "\n$ ", 3, 0);
        
        remove(out_filename);
    }

    close(client_socket);
    return NULL;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(9999), INADDR_ANY};
    
    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 10);
    printf("Telnet Server dang chay tai port 9999...\n");

    while (1) {
        int *client_sock = malloc(sizeof(int));
        *client_sock = accept(server_fd, NULL, NULL);
        pthread_t t;
        pthread_create(&t, NULL, telnet_handler, client_sock);
        pthread_detach(t);
    }
    close(server_fd);
    return 0;
}