#include <stdio.h>
#include <winsock2.h>
#include <stdint.h>

int send_to_all(char *buffer, size_t buffsize,
                SOCKET to_skip, SOCKET self,
                fd_set set_of_receivers, uint16_t max_fd) {

    uint16_t bytes_over_network;
    for ( uint16_t index; index <= max_fd; ++index) {
        if ((index == to_skip) || (index == self) || !(FD_ISSET(index, &set_of_receivers)))
            continue;
        bytes_over_network = send(index, buffer, (int) buffsize, 0);
        if (bytes_over_network <= 0)
            return bytes_over_network;
    }
    return 1;
}

int main() {
    uint8_t error_code, set_sock_option = 1;
    struct WSAData setup;
    SOCKET listener;
    fd_set main, reader;
    struct sockaddr_in server, client;
    uint32_t length = sizeof(struct sockaddr_in);
    uint16_t max_fd, new_fd, client_count = 0;
    uint64_t bytes_through_network;
    #define PORT 1234
    #define MAX_BACKLOG_SIZE 5
    #define BUFFSIZE 256
    char *buffer = malloc( BUFFSIZE * sizeof(char));
    if (WSAStartup(MAKEWORD(2, 2), &setup) < 0) {
        perror("Error establishing WSAStartup\n");
        error_code = 1;
        goto cleanup_skip_wsa;
    }
    /// Create socket
    printf("WSA established\n");
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if ((long long) listener < 0) {
        perror("Error socketing\n");
        error_code = 2;
        goto cleanup;
    }
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (const char *) &set_sock_option, sizeof set_sock_option) < 0) {
        perror("Error setting socket options\n");
        error_code = 3;
        goto cleanup;
    }
    printf("Socket created\n");
    memset(&server, 0, length);
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    if (bind(listener, (struct sockaddr * ) & server, (int) length) < 0) {
        perror("Error binding\n");
        error_code = 4;
        goto cleanup;
    }
    printf("Bind successful\n");
    if (listen(listener, MAX_BACKLOG_SIZE) < 0) {
        perror("Error listening\n");
        error_code = 5;
        goto cleanup;
    }
    printf("Listening on port %d\n", PORT);
    FD_ZERO(&main);
    FD_ZERO(&reader);
    FD_SET(listener, &main);
    max_fd = listener;
    memset(&client, 0, length);
    while (true) {
        reader = main;
        if (select(max_fd + 1, &reader, nullptr, nullptr, nullptr) < 0) {
            perror("Error selecting\n");
            error_code = 6;
            break;
        }

        for (SOCKET current = 0; current <= max_fd; ++current) {
            if (!FD_ISSET(current, &reader))
                continue;
            if (current == listener) {
                /// got a new connection
                new_fd = accept(listener, (struct sockaddr * ) & client, (int *) (&length));
                if (new_fd < 0) {
                    perror("Error accepting\n");
                    error_code = 7;
                    goto cleanup;
                }
                ++client_count;
                FD_SET(new_fd, &main);
                max_fd = (max_fd >= new_fd) ? max_fd : new_fd;
                printf("New connection established with %hu\n", new_fd);
                sprintf(buffer, "Everybody welcome %hu\n", new_fd);
                send_to_all(buffer, strlen(buffer), new_fd, listener, main, max_fd);
            } else {
                /// got new data from a client
                memset(buffer, 0, BUFFSIZE);
                bytes_through_network = recv(current, buffer, BUFFSIZE, 0);
                if (bytes_through_network > 0) {
                    if (bytes_through_network < BUFFSIZE)
                        buffer[bytes_through_network] = '\0';
                    if (strncmp(buffer, "quit", strlen("quit")) != 0) {
                        char *temp = (char *) malloc(BUFFSIZE * sizeof (char));
                        sprintf(temp, "[%llu]:- %s\n", current, buffer);
                        strcpy(buffer, temp);
                        free(temp);
                        send_to_all(buffer, strlen(buffer), current, listener, main, max_fd);
                    } else {
                        printf("Client [%llu] left the chatroom\n", current);
                        bytes_through_network = send(current, "Request granted. Closing connection...\n", 38, 0);
                        if (bytes_through_network <= 0) {
                            perror("Error sending message\n");
                            error_code = 9;
                            goto cleanup;
                        }
                        closesocket(current);
                        FD_CLR(current, &main);
                        --client_count;
                        sprintf(buffer, "Client %llu disconnected\n", current);
                        send_to_all(buffer, strlen(buffer), current, listener, main, max_fd);
                        break;
                    }
                } else {
                    if (bytes_through_network == 0) {
                        printf("Client %llu forcibly hung up\n", current);
                        send_to_all(buffer, strlen(buffer), current, listener, main, max_fd);
                    } else {
                        perror("Error reading\n");
                    }
                    --client_count;
                    closesocket(current);
                    FD_CLR(current, &main);
                }
            }
        }
    }
    cleanup:
    WSACleanup();
    cleanup_skip_wsa:
    free(buffer);
    #undef PORT
    #undef MAX_BACKLOG_SIZE
    #undef BUFFSIZE
    exit(error_code);
}
