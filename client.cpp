#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_BUF 256

int client_sock;
char disconnect_msg[] = "Goodbye!";

void* process_messages(void* param) {

    while (true) {
        int bytes_read;
        char buffer[MAX_BUF];
        bytes_read = read(client_sock, buffer, sizeof(buffer));

        printf("%s\n", buffer);
        if (bytes_read < MAX_BUF) {
            buffer[bytes_read] = '\0';
        }

        if (strcmp(buffer, disconnect_msg) == 0) {
            break;
        }
        memset(buffer, 0, sizeof(buffer));
    }

    return NULL;
}

int main() {
    struct sockaddr_in server_info;
    pthread_t msg_thread;

    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        perror("socket");
        exit(1);
    }

    server_info.sin_family = AF_INET;
    server_info.sin_port = htons(2345);
    inet_pton(AF_INET, "127.0.0.1", &server_info.sin_addr);

    if (connect(client_sock, (struct sockaddr*)&server_info, sizeof(server_info)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Connected to the server!\n");

    pthread_create(&msg_thread, NULL, process_messages, NULL);

    while (true) {
        char input[MAX_BUF];
        fgets(input, MAX_BUF, stdin);
        int input_len = strlen(input) - 1;

        send(client_sock, input, input_len, 0);
        usleep(10000);
    }

    return 0;
}
