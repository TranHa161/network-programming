#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct {
    int s1;
    int s2;
} ClientPair;

void* relay_messages(void* arg) {
    ClientPair *pair = (ClientPair*)arg;
    int src = pair->s1;
    int dest = pair->s2;
    char buffer[1024];

    while (1) {
        int bytes = recv(src, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;
        
        if (send(dest, buffer, bytes, 0) <= 0) break;
    }

    printf("Mot client ngat ket noi. Dang dong cap chat...\n");
    close(src);
    close(dest);
    free(pair);
    return NULL;
}

int main() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(8080), INADDR_ANY};
    
    bind(server_sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_sock, 10);

    printf("Chat Server dang cho cac cap ket noi...\n");

    int waiting_client = -1;

    while (1) {
        int client_sock = accept(server_sock, NULL, NULL);
        printf("Co client moi ket noi!\n");

        if (waiting_client == -1) {
            waiting_client = client_sock;
            send(client_sock, "Waiting for a partner...\n", 25, 0);
        } else {
            printf("Ghep cap thanh cong!\n");
            send(waiting_client, "Partner found! Start chatting.\n", 31, 0);
            send(client_sock, "Partner found! Start chatting.\n", 31, 0);

            ClientPair *p1 = malloc(sizeof(ClientPair));
            p1->s1 = waiting_client; p1->s2 = client_sock;
            
            ClientPair *p2 = malloc(sizeof(ClientPair));
            p2->s1 = client_sock; p2->s2 = waiting_client;

            pthread_t t1, t2;
            pthread_create(&t1, NULL, relay_messages, p1);
            pthread_create(&t2, NULL, relay_messages, p2);

            pthread_detach(t1);
            pthread_detach(t2);

            waiting_client = -1;
        }
    }
    return 0;
}