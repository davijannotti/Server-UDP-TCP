CC = gcc
CFLAGS = -Wall

SERVER = server_tcp
CLIENT = client_tcp

# Definindo o nome do arquivo a ser compilado
SERVER_SRC = servidor_tcp.c
CLIENT_SRC = cliente_tcp.c

# Compilação dos arquivos
all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_SRC)

$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_SRC)

# Regra para rodar o servidor
run_server: $(SERVER)
	@echo "Iniciando o servidor TCP..."
	./$(SERVER)

# Regra para rodar o cliente
run_client: $(CLIENT)
	@echo "Iniciando o cliente TCP..."
	./$(CLIENT)

# Limpeza de arquivos compilados
clean:
	@echo "Limpando arquivos compilados..."
	rm -f $(SERVER) $(CLIENT)

.PHONY: all run_server run_client clean
