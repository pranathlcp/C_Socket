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
#include <time.h>

int socket_list[50];

// START QUEUE IMPLEMENTATION

#define QUEUE_LENGTH 50

struct message {
    int socket_id;
    char message[2000];
    int timestamp;
    bool pushed;
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

// END QUEUE IMPLEMENTATION

//Thread Function
void *reader_thread_function(void *arg)
{
    printf("New Reader Thread Started!\n");
    char client_message[2000];
    int new_socket = *((int *)arg);
    int receive_message;

    memset(client_message, 0, 2000);
    while ( (receive_message = recv(new_socket , client_message , 2000 , 0)) > 0 ) {
        //printf("Socket ID [%d] Says: %s", new_socket, client_message);

        if ( receive_message <= 0) {
            perror("Error Receiving the Message");
        }

        struct message new_message;
        strcpy(new_message.message, client_message);
        new_message.socket_id = new_socket;
        new_message.timestamp = (int) time(NULL);
        new_message.pushed = false;
        enqueue(new_message);
        memset(client_message, 0, 2000);
    }

    return NULL;
}

void send_to_all(char message[2100], int socket_id) {
    for (int i = 0; i <= (sizeof(socket_list) / sizeof(socket_list[0]) - 1); i++) {
        if (socket_list[i] != socket_id) {
            send(socket_list[i], message, sizeof(message), 0);
        }
    }
}

void *writer_thread_function(void *arg) {
    printf("New Writer Thread Started!\n");

    while (1 == 1) {
        if (!isEmpty()) {
            for (int i = front; i <= rear; i++) {
                if (message_array[i].pushed == false) {
                    char message_with_id[2100];
                    sprintf(message_with_id, "Socket ID [%d] Says: %s\n", message_array[i].socket_id,
                            message_array[i].message);
                    send_to_all(message_with_id, message_array[i].socket_id);
                    dequeue();
                }
            }
        }
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