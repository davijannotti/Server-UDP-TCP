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

// Estrutura para passar argumentos para threads
typedef struct {
    int client_socket;
} thread_arg_t;

// Variáveis para o servidor com threads e fila
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int task_queue[QUEUE_SIZE], front = 0, rear = 0, count = 0;

// Padrão HTTP 1.1 para testes com Siege
const char *response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 13\r\n"
    "\r\n"
    "Hello, World!";

// Função para lidar com cada cliente (usada por threads)
void *handle_client(void *arg) {
    int client_socket = ((thread_arg_t *)arg)->client_socket;
    free(arg);

    char buf[BUF_SIZE];
    int nbytes;

    send(client_socket, response, strlen(response), 0);

    while ((nbytes = recv(client_socket, buf, sizeof(buf), 0)) > 0) {
        send(client_socket, buf, nbytes, 0);
    }

    close(client_socket);
    pthread_exit(NULL);
}

// Enfileirar tarefas (servidor com fila)
void enqueue_task(int client_fd) {
    pthread_mutex_lock(&lock);

    // Evitar sobrescrita na fila
    while (count == QUEUE_SIZE) {
        pthread_cond_wait(&cond, &lock); // Espera se a fila estiver cheia
    }
    task_queue[rear] = client_fd;
    rear = (rear + 1) % QUEUE_SIZE;
    count++;
    pthread_cond_signal(&cond);

    pthread_mutex_unlock(&lock);
}

// Workers (threads) para lidar com clientes na fila
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

        char buffer[BUF_SIZE] = {0};
        int nbytes = read(client_fd, buffer, BUF_SIZE);
        if (nbytes > 0) {
            write(client_fd, response, strlen(response));
        } else {
            perror("Read failed");
        }

        close(client_fd);
    }
    return NULL;
}

// Servidor Iterativo
void server_iterative() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);
    char buf[BUF_SIZE];

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

    if (listen(server_socket, BACKLOG) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Iterative Server running on port %d...\n", PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (client_socket == -1) {
            perror("Accept failed");
            continue;
        }
        send(client_socket, response, strlen(response), 0);

        int nbytes = recv(client_socket, buf, sizeof(buf), 0);
        if (nbytes > 0) {
            send(client_socket, buf, nbytes, 0);
        }
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

    if (listen(server_socket, BACKLOG) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Threaded Server running on port %d...\n", PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (client_socket == -1) {
            perror("Accept failed");
            continue;
        }

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
    int opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Reutilização de endereço
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
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
        if (client_fd == -1) {
            perror("Accept failed");
            continue;
        }
        enqueue_task(client_fd);
    }

    close(server_fd);
}

// Servidor Concorrente com Select
void server_with_select() {
    int listener, newfd, fdmax;
    struct sockaddr_in serveraddr, clientaddr;
    fd_set master, read_fds;
    char buf[BUF_SIZE];
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
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("Select failed");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == listener) {
                    addrlen = sizeof(clientaddr);
                    newfd = accept(listener, (struct sockaddr *)&clientaddr, &addrlen);
                    if (newfd == -1) {
                        perror("Accept failed");
                    } else {
                        FD_SET(newfd, &master);
                        if (newfd > fdmax) {
                            fdmax = newfd;
                        }
                    }
                } else {
                    int nbytes = recv(i, buf, sizeof(buf), 0);
                    if (nbytes <= 0) {
                        close(i);
                        FD_CLR(i, &master);
                    } else {
                        send(i, response, strlen(response), 0);
                        send(i, buf, nbytes, 0);
                    }
                }
            }
        }
    }
    close(listener);
}

// Main: Selecionar o tipo de servidor
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [iterative|threads|queue|select]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "iterative") == 0) {
        server_iterative();
    } else if (strcmp(argv[1], "threads") == 0) {
        server_with_threads();
    } else if (strcmp(argv[1], "queue") == 0) {
        server_with_threads_queue();
    } else if (strcmp(argv[1], "select") == 0) {
        server_with_select();
    } else {
        fprintf(stderr, "Invalid option: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    return 0;
}
