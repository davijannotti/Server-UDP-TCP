#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <time.h>

#define PORT 8080
#define FILE_NAME "received_file.bin" // Nome do arquivo recebido
#define BUFFER_SIZE 1024

void start_tcp_client();

int main() {
    start_tcp_client();
    return 0;
}

void start_tcp_client() {
    int client_fd;
    struct sockaddr_in server_addr;
    FILE *file;
    char buffer[BUFFER_SIZE];
    size_t bytes_received;
    long file_size;
    clock_t start, end;
    double total_time, download_rate;

    // Criar o socket TCP
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Falha ao criar socket TCP");
        exit(EXIT_FAILURE);
    }

    // Configurar o endereço do servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Conectar ao servidor
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Falha ao conectar ao servidor");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    printf("Conectado ao servidor TCP.\n");

    // Receber o tamanho do arquivo
    read(client_fd, &file_size, sizeof(file_size));
    printf("Tamanho do arquivo: %ld bytes\n", file_size);

    // Abrir o arquivo para salvar os dados recebidos
    file = fopen(FILE_NAME, "wb");
    if (!file) {
        perror("Falha ao criar o arquivo");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // Medir o tempo de transferência
    start = clock();

    // Receber os pacotes do servidor e salvar no arquivo
    size_t total_bytes_received = 0;
    while ((bytes_received = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
        total_bytes_received += bytes_received;
    }

    end = clock();
    total_time = (double)(end - start) / CLOCKS_PER_SEC;
    download_rate = total_bytes_received / total_time;

    printf("Transferência concluída.\n");
    printf("Tamanho do arquivo recebido: %ld bytes\n", total_bytes_received);
    printf("Tempo total: %.2f segundos\n", total_time);
    printf("Taxa de download: %.2f bytes/segundo\n", download_rate);

    fclose(file);
    close(client_fd);
}
