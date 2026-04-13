#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Sử dụng: %s <port_s> <ip_d> <port_d>\n", argv[0]);
        return 1;
    }

    int port_s = atoi(argv[1]);
    char *ip_d = argv[2];
    int port_d = atoi(argv[3]);

    // 1. Tạo socket UDP
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == -1) {
        perror("socket() failed");
        return 1;
    }

    // 2. Chuyển socket sang chế độ Non-blocking
    unsigned long ul = 1;
    ioctl(sockfd, FIONBIO, &ul);

    // 3. Chuyển stdin sang Non-blocking
    unsigned long ul_stdin = 1;
    ioctl(STDIN_FILENO, FIONBIO, &ul_stdin);

    // 4. Thiết lập địa chỉ nhận
    struct sockaddr_in addr_s = {0};
    addr_s.sin_family = AF_INET;
    addr_s.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_s.sin_port = htons(port_s);

    if (bind(sockfd, (struct sockaddr *)&addr_s, sizeof(addr_s))) {
        perror("bind() failed");
        return 1;
    }

    // 5. Thiết lập địa chỉ gửi
    struct sockaddr_in addr_d = {0};
    addr_d.sin_family = AF_INET;
    addr_d.sin_addr.s_addr = inet_addr(ip_d);
    addr_d.sin_port = htons(port_d);

    printf("UDP Chat đang chạy (Port nhận: %d, Gửi đến: %s:%d)\n", port_s, ip_d, port_d);
    printf("Nhập tin nhắn để gửi...\n----------------------------\n");

    char buf[1024];
    struct sockaddr_in remote_addr;
    socklen_t remote_len = sizeof(remote_addr);

    while (1) {
        int ret = recvfrom(sockfd, buf, sizeof(buf) - 1, 0, 
                           (struct sockaddr *)&remote_addr, &remote_len);
        if (ret > 0) {
            buf[ret] = 0;
            printf("\n[Nhận]: %s\n> ", buf);
            fflush(stdout);
        }

        if (fgets(buf, sizeof(buf), stdin) != NULL) {
            buf[strcspn(buf, "\n")] = 0;
            
            if (strlen(buf) > 0) {
                sendto(sockfd, buf, strlen(buf), 0, 
                       (struct sockaddr *)&addr_d, sizeof(addr_d));
                printf("> ");
                fflush(stdout);
            }
        }
    }

    close(sockfd);
    return 0;
}