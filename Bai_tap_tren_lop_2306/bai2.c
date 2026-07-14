#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h> // Thêm thư viện này để dùng struct in_addr và inet_ntoa

int main() 
{ 
    int sock_raw = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock_raw < 0)
    {
        printf("Failed to create socket: %d\n", errno);
        return 1;
    }

    unsigned char *buffer = (unsigned char *)malloc(65535);
    int data_size;
    struct sockaddr saddr;
    sxsxs
    socklen_t saddr_size = sizeof(saddr); 

    unsigned char ip_protocol;

    while (1)
    {
        data_size = recvfrom(sock_raw, buffer, 65535, 0, &saddr, &saddr_size);
        if (data_size < 0)
        {
            printf("Recvfrom error, failed to get packets\n");
            free(buffer);
            return 1;
        }
        
        printf("-----------------------------------------\n");
        printf("Data size: %d bytes\n", data_size);

        // --- PHẦN CẬP NHẬT: Trích xuất và hiển thị IP Nguồn / IP Đích ---
        struct in_addr source_ip, dest_ip;
        
        // Copy 4 byte IP nguồn (từ offset 12) và 4 byte IP đích (từ offset 16)
        memcpy(&source_ip.s_addr, buffer + 12, 4);
        memcpy(&dest_ip.s_addr, buffer + 16, 4);

        // Sử dụng inet_ntoa để chuyển đổi định dạng byte sang chuỗi dạng "X.X.X.X" dễ đọc
        // Lưu ý: inet_ntoa dùng chung một buffer tĩnh, nên cần in riêng biệt hoặc copy ra chỗ khác
        printf("Source IP:      %s\n", inet_ntoa(source_ip));
        printf("Destination IP: %s\n", inet_ntoa(dest_ip));

        // Kiểm tra Protocol (offset 9)
        memcpy(&ip_protocol, buffer + 9, 1);
        if (ip_protocol == 1)
            printf("Protocol:       ICMP\n");
        else if (ip_protocol == 6)
            printf("Protocol:       TCP\n");
        else if (ip_protocol == 17)
            printf("Protocol:       UDP\n");  
        else
            printf("Protocol:       Other (%d)\n", ip_protocol);

        // In 40 byte đầu tiên dưới dạng Hex để debug
        printf("Hex dump (first 40 bytes):\n");
        for (int i = 0; i < 40 && i < data_size; i++)
            printf("%02x ", buffer[i]);
        printf("\n");
    }

    close(sock_raw);
    free(buffer);
    return 0;
}