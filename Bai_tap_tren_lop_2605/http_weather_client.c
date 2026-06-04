#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

int main() {
    // 1. Khởi tạo socket
    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client < 0) {
        perror("Không thể tạo socket");
        return 1;
    }

    // 2. Phân giải tên miền (Chỉ truyền HOSTNAME vào đây)
    struct addrinfo *res = NULL;
    int status = getaddrinfo("api.weatherapi.com", "80", NULL, &res);
    if (status != 0 || res == NULL) {
        fprintf(stderr, "getaddrinfo thất bại: %s\n", gai_strerror(status));
        close(client);
        return 1;
    }

    // 3. Kết nối tới server
    if (connect(client, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Kết nối thất bại");
        freeaddrinfo(res);
        close(client);
        return 1;
    }
    freeaddrinfo(res); // Giải phóng ngay sau khi kết nối thành công

    // 4. Chuẩn bị HTTP GET Request chuẩn chỉnh
    // Phần Path + Query nằm ở dòng đầu tiên của Request
    char *request = "GET /v1/current.json?key=48bab0abac324847925230945232306&q=Hanoi&aqi=no HTTP/1.1\r\n"
                    "Host: api.weatherapi.com\r\n"
                    "User-Agent: C-HttpClient/1.0\r\n"
                    "Connection: close\r\n\r\n";
    
    send(client, request, strlen(request), 0);

    // 5. Nhận dữ liệu (Dùng vòng lặp để nhận toàn bộ response)
    char buf[8192]; // Tăng kích thước buffer cho an toàn
    int total_len = 0;
    int received = 0;

    while ((received = recv(client, buf + total_len, sizeof(buf) - total_len - 1, 0)) > 0) {
        total_len += received;
        if (total_len >= sizeof(buf) - 1) break;
    }
    buf[total_len] = '\0'; // Đảm bảo kết thúc chuỗi

    // In toàn bộ kết quả thô nhận được (bao gồm HTTP Header và JSON) để debug
    printf("--- HTTP RESPONSE ---\n%s\n---------------------\n\n", buf);

    // 6. Tách chuỗi bằng strstr (Đã bọc kiểm tra an toàn)
    char *start = strstr(buf, "\"temp_c\""); // Tìm chính xác cụm "temp_c"
    if (start) {
        // Dịch con trỏ qua chuỗi `"temp_c":` để đến phần số thực
        // Ví dụ: "temp_c":25.0, -> cần nhảy qua `"temp_c":` (8 ký tự)
        start = strchr(start, ':');
        if (start) {
            start++; // Nhảy qua dấu ':'
            
            char *end = strchr(start, ',');
            if (end && (end > start)) {
                char temp[16];
                size_t len_temp = end - start;
                if (len_temp < sizeof(temp)) {
                    strncpy(temp, start, len_temp);
                    temp[len_temp] = '\0';
                    printf("==> Nhiệt độ hiện tại ở Hà Nội là: %s độ C\n", temp);
                }
            }
        }
    } else {
        printf("Không tìm thấy thông tin 'temp_c' trong dữ liệu trả về.\n");
    }

    // 7. Dọn dẹp
    close(client);
    return 0;
}