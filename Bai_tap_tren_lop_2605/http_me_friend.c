#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 8080

void send_html_response(int client_socket, const char *status, const char *html_body) {
    char response[2048];

    sprintf(response,
            "HTTP/1.1 %s\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            status, strlen(html_body), html_body);
            
    send(client_socket, response, strlen(response), 0);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Tạo socket thất bại");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt thất bại");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind thất bại");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen thất bại");
        exit(EXIT_FAILURE);
    }

    printf("Server đang chạy tại cổng: http://localhost:%d\n", PORT);
    printf("Sẵn sàng nhận request...\n\n");

    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept thất bại");
            continue;
        }

        char buffer[2048] = {0};
        read(client_socket, buffer, sizeof(buffer) - 1);
        
        printf("--- Nhận Request ---\n%s\n", buffer);

        if (strstr(buffer, "GET /me ") == buffer || strstr(buffer, "GET /me HTTP")) {
            const char *html_me = "<html><body>"
                                  "<h1>Thông tin cá nhân</h1>"
                                  "<p>Họ và tên: <strong>Lý Hà Trân Trân</strong></p>"
                                  "<p>MSSV: 20235440</p>"
                                  "</body></html>";
            send_html_response(client_socket, "200 OK", html_me);
            
        } else if (strstr(buffer, "GET /friend ") == buffer || strstr(buffer, "GET /friend HTTP")) {
            const char *html_friend = "<html><body>"
                                      "<h1>Thông tin bạn bè</h1>"
                                      "<p>Họ và tên: <strong>Trần Thị B</strong></p>"
                                      "<p>MSSV: 20215678</p>"
                                      "</body></html>";
            send_html_response(client_socket, "200 OK", html_friend);
            
        } else {
            const char *html_error = "<html><body>"
                                     "<h1>404 Not Found</h1>"
                                     "<p style='color:red;'>Đường dẫn không hợp lệ. Vui lòng truy cập /me hoặc /friend</p>"
                                     "</body></html>";
            send_html_response(client_socket, "404 Not Found", html_error);
        }

        close(client_socket);
        printf("--------------------\n\n");
    }
    
    close(server_fd);
    return 0;
}