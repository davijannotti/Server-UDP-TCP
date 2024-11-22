#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>

#define PORT 8080
#define FILE_NAME "file.bin" // Nome do arquivo a ser enviado
#define BUFFER_SIZE 1024

void start_tcp_server();

unsigned long calculate_checksum(FILE *file);

int main() {
    start_tcp_server();
    return 0;
}

void start_tcp_server() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    FILE *file;
    size_t bytes_read;
    long file_size;
    unsigned long checksum;

    // Criar o socket TCP
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Falha ao criar socket TCP");
        exit(EXIT_FAILURE);
    }

    // Configurar o endereço
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Vincular o socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Falha ao vincular o socket TCP");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Escutar conexões
    if (listen(server_fd, 5) < 0) {
        perror("Falha ao escutar no socket");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor TCP esperando conexões...\n");

    // Aceitar conexão do cliente
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_fd < 0) {
        perror("Falha ao aceitar conexão");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Cliente conectado.\n");

    // Abrir o arquivo para leitura
    file = fopen(FILE_NAME, "rb");
    if (!file) {
        perror("Falha ao abrir o arquivo");
        close(client_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Obter o tamanho do arquivo
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Calcular o checksum do arquivo
    checksum = calculate_checksum(file);
    fseek(file, 0, SEEK_SET);

    // Enviar o tamanho do arquivo e o checksum para o cliente
    write(client_fd, &file_size, sizeof(file_size));
    write(client_fd, &checksum, sizeof(checksum));

    // Enviar arquivo em pacotes
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(client_fd, buffer, bytes_read, 0) < 0) {
            perror("Falha ao enviar dados");
            fclose(file);
            close(client_fd);
            close(server_fd);
            exit(EXIT_FAILURE);
        }
    }

    printf("Arquivo enviado com sucesso via TCP.\n");

    fclose(file);
    close(client_fd);
    close(server_fd);
}

unsigned long calculate_checksum(FILE *file) {
    unsigned long checksum = 0;
    unsigned char byte;

    while (fread(&byte, 1, 1, file) == 1) {
        checksum += byte;
    }

    return checksum;
}
