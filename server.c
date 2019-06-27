//
// Created by pranath on 6/27/19.
//

#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>

#include <unistd.h>
#include <string.h>
#include <pthread.h>

//Thread Function
void *msgThreadFunction(void *arg)
{
    printf("New Thread Started!\n");
    char client_message[2000];
    int new_socket = *((int *)arg);
    int receive_message;

    while ( (receive_message = recv(new_socket , client_message , 2000 , 0)) > 0 ) {
        //printf("Socket ID [%d] Says: %s", new_socket, client_message);
        write(new_socket, client_message, sizeof(client_message));
        //
        memset(client_message, 0, 2000);
    }
    return NULL;
}

int main () {

    int client_socket;
    char server_message[256] = "You have reached the server\n";

    // Create the server socket
    int server_socket;

    server_socket = socket (AF_INET, SOCK_STREAM, 0);

    if (server_socket == -1) {
        printf("Problem with creating socket\n");
    } else {
        printf("Socket created successfully\n");
    }

    // Define the Server Address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(9002);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to specified IP and Port

    int bind_result = bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));

    if (bind_result < 0 ) {
        perror("Binding Error");
        return 0;
    }

    printf("Bind operation completed successfully\n");

    listen(server_socket, 10);
    printf("Listening for incoming connection\n");

    // Accept incoming connections
    while ( (client_socket = accept(server_socket, NULL, NULL)) > 0 )  {
        printf("New Client Connected\n");
        send(client_socket, server_message, sizeof(server_message), 0);
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, msgThreadFunction, &client_socket);
        //pthread_join(thread_id, NULL);
    }

    close(server_socket);

    return 0;
}