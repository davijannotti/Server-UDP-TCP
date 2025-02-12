#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>

#define PORT 8080
#define FILE_NAME "received_file.bin"  // Nome do arquivo a ser salvo
#define PACKET_SIZE 1024  // Tamanho do pacote
#define TIMEOUT 2         // Tempo limite para ACKs (segundos)

typedef struct {
    int seq_num;          // Número de sequência do pacote
    int data_size;        // Tamanho real dos dados no pacote
    char data[PACKET_SIZE]; // Dados do arquivo
} Packet;

void start_udp_server();

int main() {
    start_udp_server();
    return 0;
}

void start_udp_server() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    Packet packet;
    FILE *file;
    int last_seq_num = -1; // Último número de sequência recebido corretamente

    // Criar socket UDP
    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        perror("Falha ao criar socket UDP");
        exit(EXIT_FAILURE);
    }

    // Configurar endereço do servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Vincular o socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Falha ao vincular o socket UDP");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor UDP aguardando pacotes...\n");

    // Abrir arquivo para escrita binária
    file = fopen(FILE_NAME, "wb");
    if (!file) {
        perror("Erro ao abrir arquivo para escrita");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Receber tamanho do arquivo
    int file_size, ack;
    while (1) {
            recvfrom(server_fd, &packet, sizeof(Packet), 0, (struct sockaddr *)&client_addr, &client_addr_len);
            // Extrair tamanho do arquivo do primeiro pacote
            if (packet.data_size == sizeof(int)) {
                memcpy(&file_size, packet.data, sizeof(int));
                ack = packet.seq_num;

                // Enviar ACK de confirmação
                sendto(server_fd, &ack, sizeof(int), 0, (struct sockaddr *)&client_addr, client_addr_len);
                printf("Tamanho do arquivo recebido: %d bytes\n", file_size);
                break;
            }
        }

    // Receber pacotes de dados
    while (1) {
        ssize_t bytes_received = recvfrom(server_fd, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &client_addr_len);

        if (bytes_received < 0) {
            perror("Erro ao receber pacote");
            continue;
        }

        // Se for o pacote de finalização, encerrar
        if (packet.data_size == sizeof("FIM") && strncmp(packet.data, "FIM", 3) == 0) {
            printf("Recebido pacote de finalização.\n");
            // Enviar ACK
            sendto(server_fd, &packet.seq_num, sizeof(int), 0, (struct sockaddr *)&client_addr, client_addr_len);
            break;
        }

        // Verificar duplicação de pacotes
        if (packet.seq_num <= last_seq_num) {
            printf("Pacote duplicado (%d), descartando.\n", packet.seq_num);
        } else {
            // Escrever dados no arquivo
            fwrite(packet.data, 1, packet.data_size, file);
            last_seq_num = packet.seq_num;
            printf("Pacote %d recebido e gravado (%d bytes).\n", packet.seq_num, packet.data_size);
        }

        // Enviar ACK
        sendto(server_fd, &packet.seq_num, sizeof(int), 0, (struct sockaddr *)&client_addr, client_addr_len);
    }

    printf("\nArquivo recebido com sucesso e salvo como '%s'\n", FILE_NAME);

    fclose(file);
    close(server_fd);
}
