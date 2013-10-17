CC=gcc
SRC=src/main.c src/sec.c
OBJ=$(SRC:.c=.o)
PRG=bin/suddenchat

all: $(PRG)

$(OBJ): %.o: %.c
	$(CC) -c -o $@ $<

$(PRG): $(OBJ)
	$(CC) $(OBJ) -o $(PRG)
