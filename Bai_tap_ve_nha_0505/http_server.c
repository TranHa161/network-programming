#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>

#define PORT 8080
#define NUM_CHILDREN 5

void handle_http_request(int client_sock) {
    char buf[2048];
    int ret = recv(client_sock, buf, sizeof(buf) - 1, 0);
    if (ret > 0) {
        buf[ret] = 0;
        printf("--- Request từ Client (PID %d) ---\n%s\n", getpid(), buf);

        char *msg = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html; charset=UTF-8\r\n"
                    "Content-Length: 40\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "<html><body><h1>Xin chao!</h1></body></html>";
        send(client_sock, msg, strlen(msg), 0);
    }
    close(client_sock);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    listen(listener, 20);
    printf("HTTP Server (Preforking) dang chay tai port %d...\n", PORT);

    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            while (1) {
                int client = accept(listener, NULL, NULL);
                if (client < 0) continue;

                printf("[Process %d] Dang xử lý kết nối...\n", getpid());
                handle_http_request(client);
            }
        }
    }

    while (wait(NULL) > 0);

    close(listener);
    return 0;
}