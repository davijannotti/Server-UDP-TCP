# Variáveis
CC = gcc
CFLAGS = -Wall -pthread
TARGET = multi_server
SRCS = multi_server.c

# Alvo principal
all: $(TARGET)

# Compilação
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

# Limpeza
clean:
	rm -f $(TARGET)

# Alvo para recompilar do zero
rebuild: clean all

# Ajuda
help:
	@echo "Opções disponíveis:"
	@echo "  make          Compila o programa."
	@echo "  make clean    Remove o executável."
	@echo "  make rebuild  Limpa e recompila."
