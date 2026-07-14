#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENTS 50
#define BUFFER_SIZE 2048

typedef struct {
    int socket;
    char nickname[50];
    int is_owner; 
    int is_joined;
} Client;

Client clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
char room_topic[256] = "No Topic";
int has_owner = 0;

void send_to_client(int sock, const char *msg) {
    send(sock, msg, strlen(msg), 0);
}

void broadcast(const char *msg, int exclude_sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].is_joined && clients[i].socket != exclude_sock) {
            send_to_client(clients[i].socket, msg);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

int is_valid_nickname(const char *nick) {
    if (strlen(nick) == 0) return 0;
    for (int i = 0; nick[i] != '\0'; i++) {
        if (!((nick[i] >= 'a' && nick[i] <= 'z') || (nick[i] >= '0' && nick[i] <= '9'))) {
            return 0;
        }
    }
    return 1;
}

// Hàm xử lý từng dòng lệnh đơn lẻ sau khi tách từ bộ đệm nhận được
void process_single_command(int sock, int client_idx, char *cmd_line) {
    // Loại bỏ ký tự xuống dòng ở cuối dòng lệnh thừa (nếu có)
    cmd_line[strcspn(cmd_line, "\r\n")] = 0;
    if (strlen(cmd_line) == 0) return;

    char cmd[20], arg1[256], arg2[1024];
    memset(cmd, 0, sizeof(cmd));
    memset(arg1, 0, sizeof(arg1));
    memset(arg2, 0, sizeof(arg2));
    
    int num_args = sscanf(cmd_line, "%s %s %[^\n]", cmd, arg1, arg2);
    if (num_args <= 0) return;

    if (strcmp(cmd, "JOIN") == 0) { // --- LỆNH JOIN ---
        if (num_args < 2) {
            send_to_client(sock, "201 INVALID NICK NAME\n");
            return;
        }
        if (!is_valid_nickname(arg1)) {
            send_to_client(sock, "201 INVALID NICK NAME\n");
            return;
        }

        pthread_mutex_lock(&clients_mutex);
        int duplicate = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].is_joined && strcmp(clients[i].nickname, arg1) == 0) {
                duplicate = 1;
                break;
            }
        }

        if (duplicate) {
            send_to_client(sock, "200 NICKNAME IN USE\n");
            pthread_mutex_unlock(&clients_mutex);
            return;
        }

        strcpy(clients[client_idx].nickname, arg1);
        clients[client_idx].is_joined = 1;
        
        if (!has_owner) {
            clients[client_idx].is_owner = 1;
            has_owner = 1;
        }
        pthread_mutex_unlock(&clients_mutex);

        send_to_client(sock, "100 OK\n");

        char join_msg[512];
        sprintf(join_msg, "JOIN %s\n", arg1);
        broadcast(join_msg, sock);

    } else {
        // Kiểm tra trạng thái JOIN trước khi thực hiện các lệnh khác
        pthread_mutex_lock(&clients_mutex);
        int joined = clients[client_idx].is_joined;
        pthread_mutex_unlock(&clients_mutex);

        if (!joined) {
            send_to_client(sock, "999 UNKNOWN ERROR\n");
            return;
        }

        if (strcmp(cmd, "MSG") == 0) { // --- LỆNH MSG ---
            if (num_args < 2) {
                send_to_client(sock, "999 UNKNOWN ERROR\n");
                return;
            }
            char *msg_content = cmd_line + 4; 
            send_to_client(sock, "100 OK\n");

            char b_msg[1500];
            sprintf(b_msg, "MSG %s %s\n", clients[client_idx].nickname, msg_content);
            broadcast(b_msg, sock);

        } else if (strcmp(cmd, "PMSG") == 0) { // --- LỆNH PMSG ---
            if (num_args < 3) {
                send_to_client(sock, "999 UNKNOWN ERROR\n");
                return;
            }
            int target_sock = -1;
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].is_joined && strcmp(clients[i].nickname, arg1) == 0) {
                    target_sock = clients[i].socket;
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);

            if (target_sock == -1) {
                send_to_client(sock, "202 UNKNOWN NICKNAME\n");
            } else {
                send_to_client(sock, "100 OK\n");
                char p_msg[1500];
                sprintf(p_msg, "PMSG %s %s\n", clients[client_idx].nickname, arg2);
                send_to_client(target_sock, p_msg);
            }

        } else if (strcmp(cmd, "OP") == 0) { // --- LỆNH OP ---
            pthread_mutex_lock(&clients_mutex);
            if (!clients[client_idx].is_owner) {
                send_to_client(sock, "203 DENIED\n");
                pthread_mutex_unlock(&clients_mutex);
                return;
            }
            int target_idx = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].is_joined && strcmp(clients[i].nickname, arg1) == 0) {
                    target_idx = i;
                    break;
                }
            }
            
            if (target_idx == -1) {
                send_to_client(sock, "202 UNKNOWN NICKNAME\n");
                pthread_mutex_unlock(&clients_mutex);
            } else {
                clients[client_idx].is_owner = 0;
                clients[target_idx].is_owner = 1;
                pthread_mutex_unlock(&clients_mutex); 

                send_to_client(sock, "100 OK\n");
                char op_msg[512];
                sprintf(op_msg, "OP %s\n", arg1);
                broadcast(op_msg, sock); 
            }

        } else if (strcmp(cmd, "KICK") == 0) { // --- LỆNH KICK ---
            pthread_mutex_lock(&clients_mutex);
            // 1. Kiểm tra quyền chủ phòng
            if (!clients[client_idx].is_owner) {
                send_to_client(sock, "203 DENIED\n");
                pthread_mutex_unlock(&clients_mutex);
                return;
            }
            if (num_args < 2) {
                send_to_client(sock, "999 UNKNOWN ERROR\n");
                pthread_mutex_unlock(&clients_mutex);
                return;
            }

            int target_sock = -1, target_idx = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].is_joined == 1 && strcmp(clients[i].nickname, arg1) == 0) {
                    target_sock = clients[i].socket;
                    target_idx = i;
                    break;
                }
            }
            
            if (target_idx == -1) {
                // Không tìm thấy người bị kick
                send_to_client(sock, "202 UNKNOWN NICKNAME\n");
                pthread_mutex_unlock(&clients_mutex);
            } else {
                // Đổi trạng thái joined để loại trừ người bị kick ra khỏi danh sách phòng chat
                clients[target_idx].is_joined = 0;
                clients[target_idx].socket = 0;
                pthread_mutex_unlock(&clients_mutex); 

                // Khởi tạo chuỗi thông điệp KICK chuẩn giao thức
                char kick_msg[512];
                sprintf(kick_msg, "KICK %s %s\n", arg1, clients[client_idx].nickname);
                
                // Gửi riêng thông báo cho người bị kick
                send_to_client(target_sock, kick_msg); 
                
                // Gửi phản hồi duy nhất "100 OK" cho chủ phòng thực hiện lệnh
                send_to_client(sock, "100 OK\n");
                
                // Tận dụng hàm broadcast sẵn có để phát tán cho NHỮNG THÀNH VIÊN KHÁC 
                // Tham số 'sock' truyền vào giúp loại trừ chính xác chủ phòng ra, không bị nhận trùng tin nhắn KICK nữa!
                broadcast(kick_msg, sock);

                // Tiến hành đóng kết nối của client bị kick
                close(target_sock);
            }

        } else if (strcmp(cmd, "TOPIC") == 0) { // --- LỆNH TOPIC ---
            pthread_mutex_lock(&clients_mutex);
            if (!clients[client_idx].is_owner) {
                send_to_client(sock, "203 DENIED\n");
                pthread_mutex_unlock(&clients_mutex);
                return;
            }
            char *new_topic = cmd_line + 6; 
            strcpy(room_topic, new_topic);
            pthread_mutex_unlock(&clients_mutex); 

            send_to_client(sock, "100 OK\n");

            char topic_msg[1500];
            sprintf(topic_msg, "TOPIC %s %s\n", clients[client_idx].nickname, room_topic);
            broadcast(topic_msg, sock);

        } else if (strcmp(cmd, "QUIT") == 0) { // --- LỆNH QUIT ---
            send_to_client(sock, "100 OK\n");
            // Sử dụng một cờ kết thúc ép buộc vòng lặp ngoài đóng socket
            pthread_mutex_lock(&clients_mutex);
            clients[client_idx].is_joined = -1; 
            pthread_mutex_unlock(&clients_mutex);
        } else {
            send_to_client(sock, "999 UNKNOWN ERROR\n");
        }
    }
}

void *handle_client(void *arg) {
    int sock = *(int *)arg;
    free(arg);
    char stream_buffer[BUFFER_SIZE * 2] = {0};
    int stream_len = 0;
    int client_idx = -1;

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == 0) {
            clients[i].socket = sock;
            clients[i].is_joined = 0;
            clients[i].is_owner = 0;
            client_idx = i;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    char recv_buf[BUFFER_SIZE];
    while (1) {
        memset(recv_buf, 0, BUFFER_SIZE);
        int n = recv(sock, recv_buf, BUFFER_SIZE - 1, 0);
        if (n <= 0) break;

        // Đọc nối dữ liệu vào stream_buffer để xử lý phân đoạn dòng (\n)
        memcpy(stream_buffer + stream_len, recv_buf, n);
        stream_len += n;
        stream_buffer[stream_len] = '\0';

        char *line_start = stream_buffer;
        char *line_end;

        // Vòng lặp tìm kiếm và tách tất cả các dòng kết thúc bằng '\n'
        while ((line_end = strchr(line_start, '\n')) != NULL) {
            *line_end = '\0'; // Tách dòng tạm thời thành chuỗi C chuẩn
            
            process_single_command(sock, client_idx, line_start);
            
            // Kiểm tra trạng thái QUIT ép buộc rời luồng
            pthread_mutex_lock(&clients_mutex);
            int force_quit = (clients[client_idx].is_joined == -1);
            pthread_mutex_unlock(&clients_mutex);
            if (force_quit) {
                stream_len = 0; 
                break;
            }

            line_start = line_end + 1;
        }

        // Dịch chuyển phần dữ liệu dư thừa chưa nhận đủ ký tự '\n' lên đầu Buffer
        int processed_bytes = line_start - stream_buffer;
        if (processed_bytes > 0 && processed_bytes < stream_len) {
            memmove(stream_buffer, line_start, stream_len - processed_bytes);
            stream_len -= processed_bytes;
            stream_buffer[stream_len] = '\0';
        } else if (processed_bytes >= stream_len) {
            stream_len = 0;
            stream_buffer[0] = '\0';
        }

        pthread_mutex_lock(&clients_mutex);
        if (clients[client_idx].is_joined == -1) {
            pthread_mutex_unlock(&clients_mutex);
            break;
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    // Luồng giải phóng tài nguyên hệ thống khi kết thúc connection
    close(sock);
    pthread_mutex_lock(&clients_mutex);
    if (client_idx != -1 && (clients[client_idx].is_joined == 1 || clients[client_idx].is_joined == -1)) {
        char quit_msg[512];
        sprintf(quit_msg, "QUIT %s\n", clients[client_idx].nickname);
        int was_owner = clients[client_idx].is_owner;
        
        clients[client_idx].is_joined = 0;
        clients[client_idx].socket = 0;
        clients[client_idx].is_owner = 0;
        
        if (was_owner) {
            has_owner = 0;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].is_joined == 1) {
                    clients[i].is_owner = 1;
                    has_owner = 1;
                    
                    pthread_mutex_unlock(&clients_mutex);
                    char op_msg[512];
                    sprintf(op_msg, "OP %s\n", clients[i].nickname);
                    broadcast(op_msg, -1);
                    pthread_mutex_lock(&clients_mutex);
                    break;
                }
            }
        }
        pthread_mutex_unlock(&clients_mutex);
        broadcast(quit_msg, -1);
    } else {
        if (client_idx != -1) {
            clients[client_idx].socket = 0;
            clients[client_idx].is_joined = 0;
            clients[client_idx].is_owner = 0;
        }
        pthread_mutex_unlock(&clients_mutex);
    }
    return NULL;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // Cấu hình tái sử dụng địa chỉ để tránh lỗi rò rỉ Port "Address already in use" khi chạy lại test liên tục
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    listen(server_fd, 20);
    printf("Chat Server is running perfectly on port 8080...\n");

    while (1) {
        int *client_sock = malloc(sizeof(int));
        *client_sock = accept(server_fd, NULL, NULL);
        if (*client_sock >= 0) {
            pthread_t thread;
            pthread_create(&thread, NULL, handle_client, client_sock);
            pthread_detach(thread);
        } else {
            free(client_sock);
        }
    }
    close(server_fd);
    return 0;
}