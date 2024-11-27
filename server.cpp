#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <fstream>
#include <iostream>
#include <string>

#define MAX_CONN 100
#define BUF_SIZE 256

int socket_fds[MAX_CONN];
int active_clients = 0;

std::ofstream logFile("chat_log.txt", std::ios::app);

void *handle_client(void *param) {
    int socket_fd = *(int *) param;

    while (true) {
        char buffer[BUF_SIZE];
        int bytes_received = read(socket_fd, buffer, BUF_SIZE);
        if (bytes_received <= 0) {
            break;
        }
        logFile << "User " << socket_fd << ": " << buffer << std::endl;

        buffer[bytes_received] = '\0';
        printf("Message from user %d: %s\n", socket_fd, buffer);

        std::string prefix = "User " + std::to_string(socket_fd+10) + ": ";
        const char *prefix_data = prefix.c_str();
        int prefix_len = strlen(prefix_data);

        memmove(buffer + prefix_len, buffer, bytes_received);
        memcpy(buffer, prefix_data, prefix_len);

        if (strcmp(buffer, "exit") == 0) {
            const char *farewell = "Goodbye!";
            send(socket_fd, farewell, strlen(farewell), 0);
            break;
        }

        send(socket_fd, buffer, bytes_received + prefix_len, 0);

        for (int i = 0; i < active_clients; i++) {
            if (socket_fds[i] != socket_fd) {
                send(socket_fds[i], buffer, bytes_received + prefix_len, 0);
            }
        }
    }

    close(socket_fd);
    return NULL;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_address, client_address;
    socklen_t client_len = sizeof(client_address);
    pthread_t client_thread;

    std::ofstream logFile("chat_log.txt", std::ios::app);
    if (!logFile) {
        std::cerr << "Error opening log file" << std::endl;
        return -1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(2345);
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);

    if (bind(server_fd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(server_fd, MAX_CONN) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Server is running on port 2345...\n");

    while (true) {
        client_fd = accept(server_fd, (struct sockaddr *) &client_address, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        printf("New connection established: %d\n", client_fd);

        if (logFile.is_open()) {
            std::ifstream logReader("chat_log.txt");
            if (logReader.is_open()) {
                std::string line;
                while (std::getline(logReader, line)) {
                    if (line.empty()) {
                        break;
                    }
                    line += "\n";
                    send(client_fd, line.c_str(), line.size(), 0);
                }
                logReader.close();
            } else {
                std::cerr << "Failed to read log file" << std::endl;
            }
        }

        const char *greeting = "Welcome to the chat!";
        send(client_fd, greeting, strlen(greeting), 0);

        socket_fds[active_clients] = client_fd;
        active_clients++;

        pthread_create(&client_thread, NULL, handle_client, &client_fd);
    }

    return 0;
}
