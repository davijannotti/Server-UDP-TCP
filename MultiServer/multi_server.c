#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>

#define PORT 8080
#define BACKLOG 10
#define BUF_SIZE 1024
#define QUEUE_SIZE 10
#define FILE_PATH "arquivo.bin"

// Estrutura para passar argumentos para threads
typedef struct {
    int client_socket;
} thread_arg_t;

// Variáveis para o servidor com threads e fila
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int task_queue[QUEUE_SIZE], front = 0, rear = 0, count = 0;

// Cabeçalho HTTP para resposta
const char *header =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: application/octet-stream\r\n"
    "Content-Disposition: attachment; filename=arquivo.bin\r\n"
    "\r\n";

// Função para enviar o arquivo binário
void send_file(int client_socket) {
    FILE *file = fopen(FILE_PATH, "rb");
    if (!file) {
        perror("Erro ao abrir arquivo");
        return;
    }
    send(client_socket, header, strlen(header), 0);

    char buffer[BUF_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUF_SIZE, file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }
    fclose(file);
}

// Função para lidar com cada cliente (usada por threads)
void *handle_client(void *arg) {
    int client_socket = ((thread_arg_t *)arg)->client_socket;
    free(arg);
    send_file(client_socket);
    close(client_socket);
    pthread_exit(NULL);
}

// Enfileirar tarefas (servidor com fila)
void enqueue_task(int client_fd) {
    pthread_mutex_lock(&lock);
    while (count == QUEUE_SIZE) {
        pthread_cond_wait(&cond, &lock);
    }
    task_queue[rear] = client_fd;
    rear = (rear + 1) % QUEUE_SIZE;
    count++;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&lock);
}

// Worker threads para processar a fila
void *thread_worker(void *arg) {
    while (1) {
        int client_fd;
        pthread_mutex_lock(&lock);
        while (count == 0) pthread_cond_wait(&cond, &lock);
        client_fd = task_queue[front];
        front = (front + 1) % QUEUE_SIZE;
        count--;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
        send_file(client_fd);
        close(client_fd);
    }
    return NULL;
}

// Servidor Iterativo
void server_iterative() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_socket, BACKLOG == -1)){
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Iterative Server running on port %d...\n", PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        send_file(client_socket);
        close(client_socket);
    }
    close(server_socket);
}

// Servidor com Threads
void server_with_threads() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_socket, BACKLOG == -1)){
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Threaded Server running on port %d...\n", PORT);
    
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        thread_arg_t *arg = malloc(sizeof(thread_arg_t));
        arg->client_socket = client_socket;
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, arg);
        pthread_detach(tid);
    }
    close(server_socket);
}

// Servidor com Threads e Fila
void server_with_threads_queue() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, BACKLOG) == -1) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Server with Threads Queue running on port %d...\n", PORT);

    pthread_t threads[4];
    for (int i = 0; i < 4; i++) {
        pthread_create(&threads[i], NULL, thread_worker, NULL);
    }
    
    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        enqueue_task(client_fd);
    }
    close(server_fd);
}

// Servidor Concorrente com Select
void server_with_select() {
    int listener, newfd, fdmax;
    struct sockaddr_in serveraddr, clientaddr;
    fd_set master, read_fds;
    socklen_t addrlen;
    
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(PORT);
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listener, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(listener, BACKLOG) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    FD_ZERO(&master);
    FD_SET(listener, &master);
    fdmax = listener;

    printf("Concurrent Server with Select running on port %d...\n", PORT);
        
    while (1) {
        read_fds = master;
        select(fdmax + 1, &read_fds, NULL, NULL, NULL);
        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == listener) {
                    addrlen = sizeof(clientaddr);
                    newfd = accept(listener, (struct sockaddr *)&clientaddr, &addrlen);
                    FD_SET(newfd, &master);
                    if (newfd > fdmax) fdmax = newfd;
                } else {
                    send_file(i);
                    close(i);
                    FD_CLR(i, &master);
                }
            }
        }
    }
    close(listener);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [iterative|threads|queue|select]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if (strcmp(argv[1], "iterative") == 0) server_iterative();
    else if (strcmp(argv[1], "threads") == 0) server_with_threads();
    else if (strcmp(argv[1], "queue") == 0) server_with_threads_queue();
    else if (strcmp(argv[1], "select") == 0) server_with_select();
    else fprintf(stderr, "Invalid option: %s\n", argv[1]);
    return 0;
}
