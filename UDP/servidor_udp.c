#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>

#define PORT 8080
#define FILE_NAME "file.bin" // Nome do arquivo a ser enviado
#define BUFFER_SIZE 2048
#define PACKET_SIZE 2048  // Tamanho do pacote em bytes

void start_udp_server();

int main() {
    start_udp_server();
    return 0;
}

void start_udp_server() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[PACKET_SIZE];
    FILE *file;
    size_t bytes_read;
    long file_size;
    int packet_count = 0;

    // Criar o socket UDP
    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        perror("Falha ao criar socket UDP");
        exit(EXIT_FAILURE);
    }

    // Configurar o endereço
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Vincular o socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Falha ao vincular o socket UDP");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor UDP esperando solicitações...\n");

    // Receber solicitação do cliente
    recvfrom(server_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);
    printf("Solicitação recebida do cliente UDP.\n");

    // Abrir arquivo para leitura
    file = fopen(FILE_NAME, "rb");
    if (!file) {
        perror("Falha ao abrir o arquivo");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Obter o tamanho do arquivo
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Enviar arquivo em pacotes
    while ((bytes_read = fread(buffer, 1, PACKET_SIZE, file)) > 0) {
        // Enviar o número do pacote e os dados
        packet_count++;
        sendto(server_fd, buffer, bytes_read, 0, (struct sockaddr *)&client_addr, client_addr_len);
    }

    // Enviar pacote finalizando o envio com o número total de pacotes
    sprintf(buffer, "FIM %df", packet_count);
    sendto(server_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&client_addr, client_addr_len);

    printf("Arquivo enviado com sucesso via UDP. Total de pacotes enviados: %d\n", packet_count);

    fclose(file);
    close(server_fd);
}
