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

# dd if=/dev/zero of=arquivo.bin bs=1024 count=10
# dd if=/dev/zero of=arquivo.bin bs=1M count=10


# siege -c 20 -t 1M -v 127.0.0.1:8080 
# siege -c 5 -r 1 127.0.0.1:8080 
