SRC=src/main.c
OBJ=bin/main.o
PRG=bin/suddenchat

all: $(PRG)

$(PRG): $(OBJ)
	gcc -Wall $(OBJ) -o $(PRG)
	
$(OBJ): $(SRC)
	gcc -Wall -c $(SRC) -o $(OBJ)