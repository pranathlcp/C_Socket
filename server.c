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
#include <inttypes.h>

bool queue_listener = true;

// START SOCKET_LIST ARRAY IMPLEMENTATION

#define SOCKET_LIST_MAX 10
int socket_list[SOCKET_LIST_MAX];
int socket_list_item_count = 0;

bool socket_list_isFull() {
    return socket_list_item_count == SOCKET_LIST_MAX;
}

bool socket_list_isEmpty() {
    return socket_list_item_count == 0;
}

// END SOCKET_LIST ARRAY IMPLEMENTATION

// START QUEUE IMPLEMENTATION

#define QUEUE_LENGTH 2

struct message {
    int socket_id;
    char *message;
    int timestamp;
    bool pushed;
};

#define MESSAGE_INITIALIZER     {-1,NULL,0,false};

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

// START READER THREAD FUNCTION
void *reader_thread_function(void *arg)
{
    printf("New Reader Thread Started!\n");


    int new_socket = *((int *)arg);
    int receive_message_size = 0;

    char msg_size_buffer[5];

    while ( (receive_message_size = recv(new_socket , msg_size_buffer , sizeof(msg_size_buffer) , 0)) > 0 ) {

        if ( receive_message_size <= 0 ) {
            perror("Error Receiving the Message\n");
        }

        int msg_size[5]={msg_size_buffer[0] - '0', msg_size_buffer[1] - '0', msg_size_buffer[2] - '0', msg_size_buffer[3] - '0', msg_size_buffer[4] - '0'};
        char msg_size_str[5];
        int i=0;
        int index = 0;
        for (i=0; i<5; i++) {
            index += snprintf(&msg_size_str[index], 6-index, "%d", msg_size[i]);
        }

//        memset(msg_size_buffer, 0, 5);

        char* strtoumax_endptr;
        int msg_size_int = strtoumax(msg_size_str, &strtoumax_endptr, 10);

        char *buffer = NULL;
        buffer = malloc( (msg_size_int + 5)*sizeof(char) + 1);
        printf("Size of Buffer: %ld\n", (msg_size_int + 2)*sizeof(char));

        int receive_message_content_size = recv(new_socket, buffer, (msg_size_int + 5 + 2)*sizeof(char), 0);

        if (receive_message_content_size <= 0) {
            perror("Error with message");
        }

        printf("%s\n", buffer);

        struct message new_message = MESSAGE_INITIALIZER;

        new_message.message = malloc(receive_message_size + 20 + 1);
        sprintf(new_message.message, "%sSocket ID[%d] Says: ", msg_size_str, new_socket);

        strcat(new_message.message, buffer);
        printf("\n\nNew Message.Message: %s\n\n", new_message.message);
//        strcpy(new_message.message, buffer);

        new_message.socket_id = new_socket;
        new_message.timestamp = (int) time(NULL);
        new_message.pushed = false;
        enqueue(new_message);

        memset(msg_size_buffer, 0, 5);
        free(buffer);
    }

    for (int i = 0; i <= socket_list_item_count-1; i++) {
        if (socket_list[i] == new_socket) {
            socket_list_item_count--;
            for (int j = i; j <= socket_list_item_count-1; j++) {
                socket_list[j] = socket_list[j+1];
            }
        }
    }

    //free(buffer);

    printf("Client Disconnected!\n");
    printf("Number of Sockets: %d out of %d\n", socket_list_item_count, SOCKET_LIST_MAX);
    return NULL;
}
// END READER THREAD FUNCTION

void send_to_all(char *message, int socket_id) {
    for (int i = 0; i <= (sizeof(socket_list) / sizeof(socket_list[0]) - 1); i++) {
        if (socket_list[i] != socket_id) {
            send(socket_list[i], message, strlen(message), 0);
        }
    }
//    free(message);
}

/*void send_to_all(char *message, int socket_id) {
    char *msg_with_id;
    strcpy(msg_with_id, "LALALA");
    malloc(strlen(message) + 50);
    strcat(msg_with_id, message);
    for (int i = 0; i <= (sizeof(socket_list) / sizeof(socket_list[0]) - 1); i++) {
        if (socket_list[i] != socket_id) {
            send(socket_list[i], msg_with_id, strlen(msg_with_id), 0);
        }
    }
   free(msg_with_id);
}
 */

// START WRITER THREAD FUNCTION
void *writer_thread_function(void *arg) {

    printf("New Writer Thread Started!\n");

    while (queue_listener) {
        if (!isEmpty()) {
            send_to_all(message_array[front].message, message_array[front].socket_id);
            free(message_array[front].message);
            dequeue();
        }
        usleep(100000);
    }

}
// END WRITER THREAD FUNCTION

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
        perror ("Error in setsockopt\n");
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
        perror("Binding Error\n");
        return 0;
    }

    printf("Bind operation completed successfully\n");

    listen(server_socket, 10);
    printf("Listening for incoming connection\n");

    pthread_t writer_thread_id;
    pthread_create(&writer_thread_id, NULL, writer_thread_function, NULL);

    // Accept incoming connections
    while ( (client_socket = accept(server_socket, NULL, NULL)) > 0 )  {

        if(socket_list_isFull()) {
            char reject_message[150] = "Sorry, chat room is full and you are now disconnected. Please try again later..\n";
            send(client_socket, reject_message, 150, 0);
            close(client_socket);
            printf("A client just connected to the server, but the client was disconnected because the queue was full..\n");
        } else {
            printf("New Client Connected\n");
            socket_list[socket_list_item_count] = client_socket;
            socket_list_item_count++;
            printf("Number of Sockets: %d out of %d\n", socket_list_item_count, SOCKET_LIST_MAX);
//            send(client_socket, server_message, sizeof(server_message), 0);
            pthread_t thread_id;
            pthread_create(&thread_id, NULL, reader_thread_function, &client_socket);
        }
    }

    close(server_socket);

    return 0;
}