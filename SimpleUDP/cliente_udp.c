#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <time.h>

#define PORT 8080
#define FILE_NAME "received_file.bin" // Nome do arquivo recebido
#define BUFFER_SIZE 2048
#define PACKET_SIZE 2048  // Tamanho do pacote em bytes
#define TIMEOUT 5 // Tempo de timeout em segundos

void start_udp_client();

int main() {
    start_udp_client();
    return 0;
}

void start_udp_client() {
    int client_fd;
    struct sockaddr_in server_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    FILE *file;
    char buffer[BUFFER_SIZE];
    size_t bytes_received;
    int total_packets = 0;
    int packets_received = 0;
    int packet_loss = 0;
    int expected_packet_count = 0;
    clock_t start, end;
    double total_time, download_rate;
    struct timeval timeout;
    fd_set read_fds;

    // Criar o socket UDP
    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd < 0) {
        perror("Falha ao criar socket UDP");
        exit(EXIT_FAILURE);
    }

    // Configurar o endereço do servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Enviar solicitação de arquivo
    sendto(client_fd, "GET_FILE", 8, 0, (struct sockaddr *)&server_addr, server_addr_len);
    printf("Solicitação de arquivo enviada ao servidor UDP.\n");

    // Receber o tamanho do arquivo do servidor
    //recvfrom(client_fd, &expected_packet_count, sizeof(expected_packet_count), 0, (struct sockaddr *)&server_addr, &server_addr_len);

    // Abrir o arquivo para salvar os dados recebidos
    file = fopen(FILE_NAME, "wb");
    if (!file) {
        perror("Falha ao criar o arquivo");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // Definir o timeout para o select
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;

    // Medir o tempo de transferência
    start = clock();

    int retry_count = 3; // Número de tentativas para receber o tamanho esperado
    for (int attempt = 0; attempt < retry_count; attempt++) {
        sendto(client_fd, "GET_COUNT", 9, 0, (struct sockaddr *)&server_addr, server_addr_len);
                                
        bytes_received = recvfrom(client_fd, &expected_packet_count, sizeof(expected_packet_count), 0, (struct sockaddr *)&server_addr, &server_addr_len);
        if (bytes_received > 0) {
            printf("Contagem de pacotes esperados recebida: %d\n", ntohl(expected_packet_count));
            break;
        }
    
        if (attempt == retry_count - 1) {
            printf("Não foi possível obter a contagem de pacotes esperados. Estimando com base nos pacotes recebidos.\n");
            expected_packet_count = 0; // Desabilitar comparações no final
        }
    }
    
    // Receber pacotes do servidor
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);

        // Esperar pela chegada de pacotes ou pelo timeout
        int activity = select(client_fd + 1, &read_fds, NULL, NULL, &timeout);

        if (activity > 0) {
            // Receber pacote
            bytes_received = recvfrom(client_fd, buffer, PACKET_SIZE, 0, (struct sockaddr *)&server_addr, &server_addr_len);
            if (bytes_received > 0) {
                // Se o pacote contém a mensagem de fim
                if (strncmp(buffer, "FIM", 3) == 0) {
                    sscanf(buffer, "FIM %df", &expected_packet_count);
                    printf("Servidor terminou de enviar.\n");
                    break;
                }

                // Armazenar o pacote
                packets_received++;
                total_packets++;
                fwrite(buffer, 1, bytes_received, file);
            }
        } else {
            // Se houve timeout, o cliente para de esperar
            printf("Timeout atingido. %d pacotes recebidos, %d pacotes esperados.\n", packets_received, expected_packet_count);
            break;
        }
    }

    // Medir o tempo total de transferência
    end = clock();
    total_time = (double)(end - start) / CLOCKS_PER_SEC;
    download_rate = total_packets * PACKET_SIZE / total_time;

    // Calcular a perda de pacotes
    packet_loss = expected_packet_count - packets_received;
    float loss_percentage = (float)packet_loss/expected_packet_count;
    
    printf("Transferência concluída.\n");
    printf("Total de pacotes recebidos: %d\n", packets_received);
    printf("Pacotes esperados: %d\n", expected_packet_count);
    printf("Perda de pacotes: %.2f%% \n", loss_percentage);
    printf("Taxa de download: %.2f bytes/segundo\n", download_rate);

    fclose(file);
    close(client_fd);
}
