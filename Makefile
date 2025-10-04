CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
TARGET = simulador_arquivos
SOURCES = main.c sistema_arquivos.c gerenciamento_disco.c logica_central.c demonstracao_fase2.c shell_comandos.c comandos_fase4.c relatorios_fase5.c

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: clean run
