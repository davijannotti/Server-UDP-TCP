#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define PACKET_SIZE 1024  // Tamanho máximo do pacote
#define TIMEOUT 2          // Tempo de timeout (segundos)
#define MAX_ATTEMPTS INFINITY     // Número máximo de retransmissões (setado em infinito para garantir a entrega dos pacotes)
#define FILE_NAME "file.bin"  // Arquivo a ser enviado
#define LOSS_PROBABILITY 1  // Probabilidade de perda de pacote (1%)

typedef struct {
    int seq_num;           // Número de sequência do pacote
    int data_size;         // Tamanho real dos dados no pacote
    char data[PACKET_SIZE]; // Dados do arquivo
} Packet;

int retransmitted_packets = 0;
void send_file(const char *file_name);
int send_with_ack(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len, void *data, int data_size, int seq_num);

int main() {
    send_file(FILE_NAME);
    return 0;
}

void send_file(const char *file_name) {
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    FILE *file;
    Packet packet;
    ssize_t bytes_read;
    int total_packets = 0;

    // Criar socket UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configurar endereço do servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Abrir o arquivo para leitura
    file = fopen(file_name, "rb");
    if (!file) {
        perror("Erro ao abrir arquivo");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Enviando arquivo %s para o servidor...\n", file_name);

    // Obter tamanho do arquivo
    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    rewind(file);

    // Gerar número de sequência aleatório
    int seq_num = rand();

    // Criar pacote para enviar o tamanho do arquivo
    packet.seq_num = seq_num;
    packet.data_size = sizeof(int);
    memcpy(packet.data, &file_size, sizeof(int));

    // Enviar tamanho do arquivo usando Stop-and-Wait
    if (send_with_ack(sockfd, &server_addr, addr_len, &packet, sizeof(int) * 2 + packet.data_size, seq_num) == -1) {
        printf("Erro: Não foi possível enviar o tamanho do arquivo após múltiplas tentativas.\n");
        fclose(file);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Tamanho do arquivo enviado com sucesso: %d bytes\n", file_size);

    // Envio dos pacotes de dados do arquivo
    seq_num++;
    while ((bytes_read = fread(packet.data, 1, PACKET_SIZE, file)) > 0) {
        packet.seq_num = seq_num;
        packet.data_size = bytes_read;

        if (send_with_ack(sockfd, &server_addr, addr_len, &packet, sizeof(int) * 2 + bytes_read, seq_num) == -1) {
            printf("Erro: Não foi possível enviar o pacote %d após múltiplas tentativas.\n", seq_num);
            fclose(file);
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        total_packets++;
        seq_num++; // Incrementa o número de sequência
    }

    // Enviar pacote de finalização
    strcpy(packet.data, "FIM");
    packet.seq_num = seq_num;
    packet.data_size = sizeof("FIM");

    if (send_with_ack(sockfd, &server_addr, addr_len, &packet, sizeof(int) * 2 + packet.data_size, seq_num) == -1) {
        printf("Erro: Não foi possível enviar o pacote de finalização.\n");
    } else {
        printf("\nArquivo enviado com sucesso!\n");
        printf("Total de pacotes enviados: %d\n", total_packets);
        printf("Total de pacotes retransmitidos: %d\n", retransmitted_packets);
    }

    fclose(file);
    close(sockfd);
}

/**
 * Função auxiliar que envia um pacote e aguarda ACK usando o protocolo Stop-and-Wait.
 * Retorna 0 se o ACK for recebido com sucesso ou -1 se exceder as tentativas máximas.
 */
int send_with_ack(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len, void *data, int data_size, int seq_num) {
    struct timeval timeout;
    fd_set read_fds;
    int ack;
    int attempts = 0;
    double random_number;

    while (attempts < MAX_ATTEMPTS) {
        // Enviar pacote a depender da probabilidade
        random_number = (rand() % 1000) + 1;
        if(random_number > LOSS_PROBABILITY)
        sendto(sockfd, data, data_size, 0, (struct sockaddr *)server_addr, addr_len);

        // Configurar timeout para aguardar ACK
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        timeout.tv_sec = TIMEOUT;
        timeout.tv_usec = 0;

        int activity = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

        if (activity > 0) {
            // Receber ACK
            recvfrom(sockfd, &ack, sizeof(int), 0, (struct sockaddr *)server_addr, &addr_len);

            // Verificar se o ACK corresponde ao número de sequência do pacote enviado
            if (ack == seq_num) {
                printf("Pacote %d confirmado pelo servidor.\n", seq_num);
                return 0;  // ACK correto, envio bem-sucedido
            }
        }

        // Timeout ou ACK incorreto, retransmitir pacote
        attempts++;
        retransmitted_packets++;
        printf("Timeout ou ACK incorreto. Tentativa %d. Retransmitindo pacote %d...\n", attempts, seq_num);
    }

    return -1; // Falha após várias tentativas
}
