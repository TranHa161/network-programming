#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 4096

/**
 * Hàm get_param: Trích xuất giá trị của tham số từ chuỗi truy vấn.
 * Ví dụ chuỗi query: "a=10&b=5&op=add" -> Nếu tìm "b", trả về "5".
 */
void get_param(const char *query, const char *name, char *output) {
    // Tìm vị trí xuất hiện của tên tham số trong chuỗi query
    char *pos = strstr(query, name);
    if (pos) {
        pos += strlen(name) + 1; // Di chuyển con trỏ qua khỏi chữ "name=" để đến phần giá trị
        
        // Vòng lặp đọc từng ký tự của giá trị cho đến khi gặp dấu '&' hoặc khoảng trắng hoặc kết thúc chuỗi
        while (*pos && *pos != '&' && *pos != ' ') {
            *pos == '+' ? *output = ' ' : (*output = *pos);
            pos++; 
            output++;
        }
    }
    *output = '\0';
}

/**
 * Hàm handle_client: Tiếp nhận request từ trình duyệt, phân tích, tính toán và trả về HTML.
 */
void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    // Đọc dữ liệu văn bản thô (HTTP Request) từ trình duyệt gửi lên và lưu vào vùng đệm buffer
    read(client_socket, buffer, BUFFER_SIZE);

    char method[10], url[1024], version[10];
    // Tách dòng đầu tiên của HTTP Request thành 3 phần riêng biệt: phương thức, đường dẫn URL, và phiên bản HTTP
    sscanf(buffer, "%s %s %s", method, url, version);

    char query[1024] = {0};
    
    // Xử lý phương thức GET: Dữ liệu đính kèm ngay trên đường dẫn URL (ví dụ: /?a=10&b=5)
    if (strcmp(method, "GET") == 0) {
        char *q = strchr(url, '?'); // Tìm vị trí dấu hỏi chấm '?' trên URL
        if (q) strcpy(query, q + 1); // Nếu có '?', copy toàn bộ phần tham số phía sau nó vào biến query
    } 
    // Xử lý phương thức POST: Dữ liệu nằm ở phần thân của HTTP Request
    else if (strcmp(method, "POST") == 0) {
        // Dữ liệu POST nằm sau một dòng trống phân cách, ký hiệu là "\r\n\r\n"
        char *body = strstr(buffer, "\r\n\r\n");
        if (body) strcpy(query, body + 4); // Nếu tìm thấy dòng trống, dịch qua 4 ký tự để copy phần dữ liệu tham số
    }

    char a_str[32] = "0", b_str[32] = "0", op[32] = "";
    
    get_param(query, "a", a_str);
    get_param(query, "b", b_str);
    get_param(query, "op", op);

    // Chuyển đổi chuỗi ký tự số thành kiểu số thực để tính toán
    double a = atof(a_str);
    double b = atof(b_str);
    double result = 0;
    char res_str[100] = "";
    int calculated = 0;

    if (strlen(op) > 0) {
        calculated = 1;
        
        if (strcmp(op, "add") == 0 || strcmp(op, "cộng") == 0) {
            result = a + b;
            sprintf(res_str, "%.2f + %.2f = %.2f", a, b, result);
        } else if (strcmp(op, "sub") == 0 || strcmp(op, "trừ") == 0) {
            result = a - b;
            sprintf(res_str, "%.2f - %.2f = %.2f", a, b, result);
        } else if (strcmp(op, "mul") == 0 || strcmp(op, "nhân") == 0) {
            result = a * b;
            sprintf(res_str, "%.2f * %.2f = %.2f", a, b, result);
        } else if (strcmp(op, "div") == 0 || strcmp(op, "chia") == 0) {
            if (b != 0) {
                result = a / b;
                sprintf(res_str, "%.2f / %.2f = %.2f", a, b, result);
            } else {
                strcpy(res_str, "Lỗi: Không thể chia cho 0!");
            }
        } else {
            calculated = 0;
        }
    }

    char html_template[] = 
        "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
        "<!DOCTYPE html><html><head><title>Máy tính C</title></head><body>"
        "<h2>MÁY TÍNH CƠ BẢN</h2>"
        "<form method='POST' action='/'>"
        "  Số a: <input type='number' name='a' step='any' value='%s'><br><br>"
        "  Số b: <input type='number' name='b' step='any' value='%s'><br><br>"
        "  Toán tử: "
        "  <select name='op'>"
        "    <option value='add' %s>Cộng (+)</option>"
        "    <option value='sub' %s>Trừ (-)</option>"
        "    <option value='mul' %s>Nhân (*)</option>"
        "    <option value='div' %s>Chia (/)</option>"
        "  </select><br><br>"
        "  <button type='submit'>Tính</button>"
        "</form>"
        "%s"
        "</body></html>";

    char sel_add[10] = "", sel_sub[10] = "", sel_mul[10] = "", sel_div[10] = "";
    if (strcmp(op, "add") == 0) strcpy(sel_add, "selected");
    else if (strcmp(op, "sub") == 0) strcpy(sel_sub, "selected");
    else if (strcmp(op, "mul") == 0) strcpy(sel_mul, "selected");
    else if (strcmp(op, "div") == 0) strcpy(sel_div, "selected");

    char response[BUFFER_SIZE * 2]; 
    
    if (calculated) {
        char dynamic_res[256];
        sprintf(dynamic_res, "<h3>Kết quả: %s</h3>", res_str);
        snprintf(response, sizeof(response), html_template, 
                 a_str, b_str, sel_add, sel_sub, sel_mul, sel_div, dynamic_res);
    } else {
        snprintf(response, sizeof(response), html_template, 
                 a_str, b_str, sel_add, sel_sub, sel_mul, sel_div, "");
    }

    write(client_socket, response, strlen(response));
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
    
    listen(server_fd, 3);

    printf("Calculator Server đang chạy tại http://localhost:%d\n", PORT);

    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        
        handle_client(client_socket);
    }
    
    return 0;
}