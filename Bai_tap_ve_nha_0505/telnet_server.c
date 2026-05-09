#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <signal.h>

#define PORT 8888
#define BACKLOG 10
#define BUFFER_SIZE 1024

int check_login(char *user, char *pass) {
    FILE *f = fopen("db.txt", "r");
    if (f == NULL) {
        perror("Lỗi mở file db.txt");
        return 0;
    }

    char db_user[50], db_pass[50];
    while (fscanf(f, "%s %s", db_user, db_pass) != EOF) {
        if (strcmp(user, db_user) == 0 && strcmp(pass, db_pass) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

void send_file_content(int client_sock, char *filename) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        char *msg = "Lỗi: Không thể mở file kết quả.\n";
        send(client_sock, msg, strlen(msg), 0);
        return;
    }

    char buffer[BUFFER_SIZE];
    while (fgets(buffer, BUFFER_SIZE, f) != NULL) {
        send(client_sock, buffer, strlen(buffer), 0);
    }
    fclose(f);
}

void handle_client(int client_sock) {
    char buffer[BUFFER_SIZE];
    char user[50], pass[50];
    int n;

    send(client_sock, "Username: ", 10, 0);
    n = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    buffer[n] = '\0';
    sscanf(buffer, "%s", user);

    send(client_sock, "Password: ", 10, 0);
    n = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    buffer[n] = '\0';
    sscanf(buffer, "%s", pass);

    if (!check_login(user, pass)) {
        char *err_msg = "Lỗi: Sai tài khoản hoặc mật khẩu. Ngắt kết nối.\n";
        send(client_sock, err_msg, strlen(err_msg), 0);
        close(client_sock);
        exit(0);
    }

    send(client_sock, "Đăng nhập thành công! Nhập lệnh:\n", 48, 0);

    while (1) {
        send(client_sock, "\n$ ", 3, 0);
        n = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;

        buffer[n] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strcmp(buffer, "exit") == 0) break;

        char command[BUFFER_SIZE + 128];
        char out_file[128];
        sprintf(out_file, "out_%d.txt", getpid());
        sprintf(command, "%s > %s 2>&1", buffer, out_file);

        system(command);

        send_file_content(client_sock, out_file);
        
        remove(out_file);
    }

    close(client_sock);
    exit(0);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Lỗi Bind");
        exit(1);
    }

    listen(server_sock, BACKLOG);
    printf("Telnet Server đang chạy trên port %d...\n", PORT);

    signal(SIGCHLD, SIG_IGN);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) continue;

        pid_t pid = fork();
        if (pid == 0) {
            close(server_sock);
            handle_client(client_sock);
        } else {
            close(client_sock);
        }
    }

    return 0;
}