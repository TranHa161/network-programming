#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <ctype.h>

void create_hust_email(char *input, char *output) {
    char name_parts[10][50];
    int count = 0;
    char mssv[20] = "";
    
    // Tách chuỗi input
    char *token = strtok(input, ", \n\r");
    while (token != NULL && count < 10) {
        // Nếu token toàn là số, đó là MSSV
        if (isdigit(token[0])) {
            strcpy(mssv, token);
        } else {
            // Chuyển về chữ thường
            for(int i = 0; token[i]; i++) token[i] = tolower(token[i]);
            strcpy(name_parts[count++], token);
        }
        token = strtok(NULL, ", \n\r");
    }

    if (count > 0 && strlen(mssv) >= 8) {
        char *main_name = name_parts[count - 1];
        
        char initials[10] = "";
        for (int i = 0; i < count - 1; i++) {
            strncat(initials, &name_parts[i][0], 1);
        }

        char mssv_suffix[10];
        sprintf(mssv_suffix, "%c%c%s", mssv[2], mssv[3], &mssv[4]);

        sprintf(output, "%s.%s%s@sis.hust.edu.vn\n", main_name, initials, mssv_suffix);
    } else {
        strcpy(output, "Dinh dang khong hop le. Vui long nhap: Ho Ten, MSSV\n");
    }
}
int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    // Chuyen socket listener sang non-blocking
    unsigned long ul = 1;
    ioctl(listener, FIONBIO, &ul);

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        close(listener);
        return 1;
    }

    if (listen(listener, 5)) {
        perror("listen() failed");
        close(listener);
        return 1;
    }

    printf("Server is listening on port 8080...\n");

    int clients[64];
    int nclients = 0;

    char buf[256];
    int len;

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client = accept(listener, (struct sockaddr *)&client_addr, &client_addr_len);
        
        if (client != -1) {
            printf("Client mới kết nối: %d\n", client);
            
            char *msg = "Vui lòng nhập 'Họ tên, MSSV': ";
            send(client, msg, strlen(msg), 0);

            ul = 1;
            ioctl(client, FIONBIO, &ul);
            
            clients[nclients++] = client;
        }

        for (int i = 0; i < nclients; i++) {
            int len = recv(clients[i], buf, sizeof(buf) - 1, 0);
            
            if (len > 0) {
                buf[len] = 0;
                printf("Nhận từ client %d: %s\n", clients[i], buf);

                char email_res[256];
                create_hust_email(buf, email_res);
                
                send(clients[i], email_res, strlen(email_res), 0);
            } 
            else if (len == 0) {
                // Client đóng kết nối
                printf("Client %d đã thoát.\n", clients[i]);
                close(clients[i]);
                clients[i] = clients[nclients - 1];
                nclients--;
                i--; 
            }
        }
    }

    close(listener);
    return 0;
}