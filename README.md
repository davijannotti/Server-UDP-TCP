# Servidor e Cliente TCP/UDP com Medição de Velocidade e Pacotes Perdidos

Este projeto implementa um **servidor** e um **cliente** em **TCP** e **UDP**. O servidor envia um arquivo de 10 MB para o cliente, enquanto o cliente mede:
- **Velocidade de download**.
- **Número de pacotes perdidos** (no caso do cliente UDP).

---

## Funcionalidades

1. **Servidor TCP/UDP**:
   - Atende requisições de clientes.
   - Envia um arquivo binário de 10 MB em partes.
   - No caso do UDP, envia um sinal de término ao final da transmissão.

2. **Cliente TCP/UDP**:
   - Solicita e recebe o arquivo do servidor.
   - Salva o arquivo recebido como `received_file.bin`.
   - Mede:
     - Velocidade de download em B/s.
     - Número total de pacotes perdidos (UDP).

3. **Medição de Pacotes Perdidos (UDP)**:
   - O cliente contabiliza o número de pacotes esperados e recebidos.
   - A diferença entre eles é exibida como pacotes perdidos.

---

## Estrutura do Projeto

- **servidor_tcp.c**: Implementação do servidor TCP.
- **cliente_tcp.c**: Implementação do cliente TCP.
- **servidor_udp.c**: Implementação do servidor UDP.
- **cliente_udp.c**: Implementação do cliente UDP.
- **Makefile**: Facilita a compilação e execução dos programas.

---

## Pré-requisitos

Certifique-se de ter as seguintes ferramentas instaladas no sistema:
- GCC (Compilador C)
- `make`
- Sistema Unix/Linux.

---

## Compilação e Execução

### 1. Compilar os programas

Use o `Makefile` para compilar o servidor e o cliente:

```bash
make
