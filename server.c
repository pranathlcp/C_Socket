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

#include <string.h>
#include <stdbool.h>

#define QUEUE_LENGTH 50

int socket_list[50];

struct message {
    int socket_id;
    char message[2000];
};

struct message message_array[QUEUE_LENGTH];
int front = 0;
int rear = -1;
int item_count = 0;

struct message front_el() {
    return message_array[front];
}

bool isEmpty() {
    return item_count == 0;
}

bool isFull() {
    return item_count == QUEUE_LENGTH;
}

int size() {
    return item_count;
}

void enqueue(struct message message) {
    if(!isFull()) {
        if(rear == QUEUE_LENGTH - 1) {
            rear = -1;
        }

        message_array[++rear] = message;
        item_count++;
    }
}

struct message dequeue() {
    struct message data = message_array[front++];

    if(front == QUEUE_LENGTH) {
        front = 0;
    }

    item_count--;
    return data;
}

//Thread Function
void *reader_thread_function(void *arg)
{
    printf("New Reader Thread Started!\n");
    char client_message[2000];
    int new_socket = *((int *)arg);
    int receive_message;

    memset(client_message, 0, 2000);
    while ( (receive_message = recv(new_socket , client_message , 2000 , 0)) > 0 ) {
        printf("Socket ID [%d] Says: %s", new_socket, client_message);

        if ( receive_message <= 0) {
            perror("Error Receiving the Message");
        }

        memset(client_message, 0, 2000);
    }

    return NULL;
}

void *writer_thread_function(void *arg) {
    printf("New Writer Thread Started!\n");
    return NULL;
}

void send_to_all(char message[2000]) {
    for(int i = 0; i <= (sizeof(socket_list)/sizeof(socket_list[0]) - 1); i++ ) {
        send(socket_list[i], message, sizeof(message), 0);
    }
}


int main () {

    int client_socket;
    int opt = 1;
    char server_message[256] = "You have reached the server\n";

    // Create the server socket
    int server_socket;

    server_socket = socket (AF_INET, SOCK_STREAM, 0);

    if (server_socket == -1) {
        printf("Problem with creating socket\n");
    } else {
        printf("Socket created successfully\n");
    }

    // Forcefully setting port to 9002
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror ("Error in setsockopt");
        exit(EXIT_FAILURE);
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

    pthread_t writer_thread_id;
    pthread_create(&writer_thread_id, NULL, writer_thread_function, NULL);

    int i = 0;
    // Accept incoming connections
    while ( (client_socket = accept(server_socket, NULL, NULL)) > 0 )  {

        printf("New Client Connected\n");
        socket_list[i] = client_socket;
        i++;
        send(client_socket, server_message, sizeof(server_message), 0);
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, reader_thread_function, &client_socket);
        //pthread_join(thread_id, NULL);
    }

    close(server_socket);

    return 0;
}