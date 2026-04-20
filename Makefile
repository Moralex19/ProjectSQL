CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
SRC = src/main.c src/lexer.c src/parser.c src/executor.c src/storage.c src/transaction.c src/process.c src/signals.c
OUT = manGUI

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

run: all
	./$(OUT)

clean:
	rm -f $(OUT)
	rm -f dbfiles/*.tmp
