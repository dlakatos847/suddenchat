CC=gcc
SRC=src/main.c src/console.c src/network.c src/cypher.c src/conversation.c
OBJ=$(SRC:.c=.o)
PRG=suddenchat

all: $(PRG) build_crypto

build_crypto:
        @dir=src/aes; target=all; $(BUILD_ONE_CMD)

$(OBJ): %.o: %.c
	$(CC) -ggdb -c -o $@ $<

$(PRG): $(OBJ)
	$(CC) -pthread $(OBJ) -o $(PRG)
