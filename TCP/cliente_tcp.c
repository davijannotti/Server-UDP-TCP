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
unsigned long calculate_checksum(FILE *file);

int main() {
    start_tcp_client();
    return 0;
}

void start_tcp_client() {
    int client_fd;
    struct sockaddr_in server_addr;
    FILE *file;
    char buffer[BUFFER_SIZE];
    size_t bytes_received, total_bytes_received = 0;
    long file_size;
    unsigned long received_checksum, computed_checksum;
    clock_t start_time, end_time;
    double transfer_time, download_rate;

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

    // Receber o tamanho do arquivo e o checksum
    read(client_fd, &file_size, sizeof(file_size));
    read(client_fd, &received_checksum, sizeof(received_checksum));

    printf("Tamanho do arquivo: %ld bytes\n", file_size);
    printf("Checksum recebido: %lu\n", received_checksum);

    // Abrir o arquivo para salvar os dados recebidos
    file = fopen(FILE_NAME, "wb");
    if (!file) {
        perror("Falha ao criar o arquivo");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // Iniciar medição do tempo de transferência
    start_time = clock();

    // Receber os pacotes do servidor e salvar no arquivo
    while ((bytes_received = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
        total_bytes_received += bytes_received;
    }

    // Finalizar medição do tempo de transferência
    end_time = clock();
    transfer_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    fclose(file);

    // Calcular o checksum do arquivo recebido
    file = fopen(FILE_NAME, "rb");
    if (!file) {
        perror("Falha ao abrir o arquivo para cálculo de checksum");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    computed_checksum = calculate_checksum(file);
    fclose(file);

    printf("Checksum calculado: %lu\n", computed_checksum);

    if (computed_checksum == received_checksum) {
        printf("Integridade dos dados verificada com sucesso!\n");
    } else {
        printf("Falha na verificação de integridade! O arquivo pode estar corrompido.\n");
    }

    // Calcular e exibir taxa de download
    download_rate = total_bytes_received / transfer_time; 
    printf("Tempo total de transferência: %.2f segundos\n", transfer_time);
    printf("Taxa de download: %.5f bytes/segundo\n", download_rate);

    close(client_fd);
}

unsigned long calculate_checksum(FILE *file) {
    unsigned long checksum = 0;
    unsigned char byte;

    while (fread(&byte, 1, 1, file) == 1) {
        checksum += byte;
    }

    return checksum;
}
