CC=gcc
SRC=src/main.c src/console.c src/network.c
OBJ=$(SRC:.c=.o)
PRG=suddenchat

all: $(PRG)

$(OBJ): %.o: %.c
	$(CC) -ggdb -c -o $@ $<

$(PRG): $(OBJ)
	$(CC) $(OBJ) -o $(PRG)
