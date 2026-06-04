#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>

#define PORT 8081
#define BUFFER_SIZE 8192
#define MAX_RESPONSE_BODY (1024 * 128) 

/**
 * Hàm get_content_type: Xác định định dạng MIME dựa vào đuôi mở rộng của file.
 * Giúp trình duyệt biết file trả về là văn bản, ảnh, nhạc hay video để hiển thị/phát cho đúng.
 */
const char* get_content_type(const char *path) {
    // Tìm vị trí dấu chấm '.' cuối cùng trong tên file để lấy đuôi file
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream"; // Nếu không có đuôi file, trả về kiểu nhị phân mặc định
    
    // So sánh đuôi file để trả về chuỗi MIME-Type tương ứng
    if (strcmp(ext, ".html") == 0) return "text/html; charset=UTF-8";
    if (strcmp(ext, ".txt") == 0 || strcmp(ext, ".c") == 0) return "text/plain; charset=UTF-8";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";
    if (strcmp(ext, ".wav") == 0) return "audio/wav";
    if (strcmp(ext, ".mp4") == 0) return "video/mp4";
    
    return "application/octet-stream"; // Kiểu nhị phân cho các định dạng lạ khác
}

/**
 * Hàm url_decode: Giải mã các ký tự đặc biệt trên URL.
 * Trình duyệt thường chuyển khoảng trắng thành '+' hoặc các ký tự tiếng Việt thành dạng hex như "%20", "%E1",...
 * Hàm này khôi phục chúng về chuỗi gốc để hệ điều hành Linux hiểu đúng đường dẫn file.
 */
void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        // Nếu gặp ký tự '%', theo sau là 2 ký tự mã Hex hợp lệ
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b))) {
            // Chuyển ký tự Hex thứ nhất sang giá trị số
            if (a >= 'a') a -= 'a'-'A';
            if (a >= 'A') a -= 'A'-10;
            else a -= '0';
            
            // Chuyển ký tự Hex thứ hai sang giá trị số
            if (b >= 'a') b -= 'a'-'A';
            if (b >= 'A') b -= 'A'-10;
            else b -= '0';
            
            // Gộp lại thành 1 ký tự ASCII (ví dụ: 16 * 2 + 0 = 32, tương ứng ký tự khoảng trắng)
            *dst++ = 16*a+b;
            src += 3; // Nhảy qua cụm "%XX" (3 ký tự)
        } else if (*src == '+') {
            *dst++ = ' '; // Nếu gặp dấu '+', giải mã thành khoảng trắng
            src++;
        } else {
            *dst++ = *src++; // Ký tự bình thường thì copy nguyên vẹn
        }
    }
    *dst = '\0'; // Đóng chuỗi kết quả
}

/**
 * Hàm handle_client: Đọc yêu cầu từ client, kiểm tra đường dẫn là thư mục hay tập tin
 * để sinh giao diện danh sách web hoặc trả về nội dung file.
 */
void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    read(client_socket, buffer, BUFFER_SIZE);

    char method[10], raw_url[1024], version[10];
    sscanf(buffer, "%s %s %s", method, raw_url, version);

    char url[1024];
    url_decode(url, raw_url);

    char path[2048];
    snprintf(path, sizeof(path), ".%s", url);

    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
        // CẬP NHẬT: Thay write bằng send kèm MSG_NOSIGNAL để bảo vệ hệ thống không bị crash đột ngột
        send(client_socket, not_found, strlen(not_found), MSG_NOSIGNAL);
        close(client_socket);
        return;
    }

    // Trường hợp 1: Đường dẫn là thư mục
    if (S_ISDIR(path_stat.st_mode)) {
        DIR *dir = opendir(path); // Mở thư mục hệ thống để bắt đầu đọc nội dung bên trong
        if (!dir) {
            char *err = "HTTP/1.1 403 Forbidden\r\n\r\n";
            send(client_socket, err, strlen(err), MSG_NOSIGNAL);
            close(client_socket);
            return;
        }
        struct dirent *entry;
        
        // CẬP NHẬT: Cấp phát động một vùng đệm đủ lớn (128KB) thay vì mảng tĩnh nhỏ, ngăn chặn hoàn toàn lỗi tràn bộ nhớ (Buffer Overflow)
        char *response_body = malloc(MAX_RESPONSE_BODY);
        if (!response_body) {
            closedir(dir);
            close(client_socket);
            return;
        }
        memset(response_body, 0, MAX_RESPONSE_BODY); // Khởi tạo vùng đệm chứa nội dung HTML trang web danh sách

        strcat(response_body, "<!DOCTYPE html><html><head><title>File Server</title><meta charset='UTF-8'></head><body>");
        strcat(response_body, "<h2>Danh sách tập tin & thư mục:</h2><ul>");

        // Nếu không phải là thư mục gốc "/", thêm một liên kết "[Trở lại ..]" để quay lại thư mục cha
        if (strcmp(url, "/") != 0) {
            strcat(response_body, "<li><a href='..'><b>[ Trở lại .. ]</b></a></li>");
        }

        // Vòng lặp đọc qua từng mục (file hoặc thư mục con) có trong thư mục hiện tại
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char full_sub_path[4096];
            snprintf(full_sub_path, sizeof(full_sub_path), "%s/%s", path, entry->d_name);
            
            struct stat sub_stat;
            if (stat(full_sub_path, &sub_stat) != 0) continue;

            char web_link[4096];
            if (strcmp(url, "/") == 0)  snprintf(web_link, sizeof(web_link), "/%s", entry->d_name);
            else                        snprintf(web_link, sizeof(web_link), "%s/%s", url, entry->d_name);

            if (strlen(response_body) + 1000 >= MAX_RESPONSE_BODY) break;

            strcat(response_body, "<li>");
            // Kiểm tra nếu mục con hiện tại là một thư mục
            if (S_ISDIR(sub_stat.st_mode)) {
                char line[2048];
                // In đậm thư mục con bằng thẻ <b>
                snprintf(line, sizeof(line), "<a href='%s'><b>[DIR] %s</b></a>", web_link, entry->d_name);
                strcat(response_body, line);
            } 
            // Nếu mục con hiện tại là một tập tin
            else {
                char line[2048];
                // In nghiêng tên file bằng thẻ <i>
                snprintf(line, sizeof(line), "<a href='%s'><i>[FILE] %s</i></a>", web_link, entry->d_name);
                strcat(response_body, line);
            }
            strcat(response_body, "</li>");
        }
        closedir(dir);
        strcat(response_body, "</ul></body></html>");

        char response_header[BUFFER_SIZE];
        snprintf(response_header, sizeof(response_header),
                 "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n", strlen(response_body));
        
        // CẬP NHẬT: Sử dụng hàm send() kết hợp cờ MSG_NOSIGNAL để gửi dữ liệu giao diện web an toàn
        send(client_socket, response_header, strlen(response_header), MSG_NOSIGNAL);
        send(client_socket, response_body, strlen(response_body), MSG_NOSIGNAL);
        
        free(response_body); // Giải phóng vùng đệm cấp phát động sau khi truyền xong
    } 
    // Trường hợp 2: Đường dẫn là file
    else {
        // Mở file dưới dạng đọc nhị phân ("rb") để xử lý an toàn cho cả file text lẫn file ảnh/audio/video
        FILE *file = fopen(path, "rb");
        if (!file) {
            // Nếu mở file thất bại (lỗi quyền truy cập), gửi HTTP 403 Forbidden
            char *err = "HTTP/1.1 403 Forbidden\r\n\r\n";
            send(client_socket, err, strlen(err), MSG_NOSIGNAL);
            close(client_socket);
            return;
        }

        // Đo kích thước (byte) của file phục vụ khai báo Header
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        char response_header[512];
        snprintf(response_header, sizeof(response_header),
                 "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n",
                 get_content_type(path), file_size);
        
        if (send(client_socket, response_header, strlen(response_header), MSG_NOSIGNAL) < 0) {
            fclose(file);
            close(client_socket);
            return;
        }

        char file_buffer[BUFFER_SIZE];
        size_t bytes_read;
        while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), file)) > 0) {
            if (send(client_socket, file_buffer, bytes_read, MSG_NOSIGNAL) < 0) {
                break; 
            }
        }
        fclose(file);
    }

    close(client_socket);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    
    listen(server_fd, 10);

    printf("File Server đang chạy tại http://localhost:%d\n", PORT);

    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);

        handle_client(client_socket);
    }
    return 0;
}