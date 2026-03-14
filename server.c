/*
 * server.c -- TCP server entry point
 *
 * Listens on PORT for incoming client connections and serves each one
 * in a persistent read loop until the client disconnects.
 *
 * Flow per client:
 *   read raw bytes -> parse command -> execute -> write response
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT        6379
#define BUFFER_SIZE 1024

/* Buffer sizes — must stay in sync with the defines in parser.c */
#define CMD_SIZE   16
#define KEY_SIZE   64
#define VALUE_SIZE 256

/* Declared in parser.c */
void parse_command(char *input, char *cmd, char *key, char *value);

/* Declared in storage.c */
char *execute_command(char *cmd, char *key, char *value);

int main(void)
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    /* Create a TCP socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    /*
     * SO_REUSEADDR: allow the port to be reused immediately after the
     * server restarts, avoiding "Address already in use" errors during
     * development.
     */
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* Bind to all interfaces on PORT */
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    /* Start listening; allow up to 5 pending connections in the backlog */
    listen(server_fd, 5);
    printf("mini-redis listening on port %d\n", PORT);

    while (1) {

        /* Block until a client connects */
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("accept");
            continue; /* non-fatal: keep waiting for the next client */
        }

        printf("client connected\n");

        /*
         * Persistent connection loop: keep reading commands from this
         * client until it closes the connection (read returns 0) or an
         * error occurs (read returns -1).
         *
         * NOTE: this inner break only exits the per-client loop.
         * The outer while(1) then calls accept() again for the next client.
         */
        while (1) {
            memset(buffer, 0, BUFFER_SIZE);

            int bytes = read(client_fd, buffer, BUFFER_SIZE - 1);
            if (bytes <= 0)
                break; /* client disconnected or read error */

            /* Strip trailing CR/LF sent by telnet or netcat */
            buffer[strcspn(buffer, "\r\n")] = '\0';

            /* Skip blank lines (e.g. lone newline from netcat) */
            if (buffer[0] == '\0')
                continue;

            /*
             * Zero-initialise so that if parse_command returns early
             * (malformed input), execute_command still receives valid
             * empty strings rather than uninitialised stack data.
             */
            char cmd[CMD_SIZE]   = {0};
            char key[KEY_SIZE]   = {0};
            char value[VALUE_SIZE] = {0};

            parse_command(buffer, cmd, key, value);

            char *response = execute_command(cmd, key, value);

            write(client_fd, response, strlen(response));
            write(client_fd, "\n", 1);
        }

        /* Close this client and loop back to accept the next one */
        close(client_fd);
        printf("client disconnected\n");
    }

    close(server_fd);
    return 0;
}
