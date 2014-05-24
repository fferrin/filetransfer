CFLAGS = -g -Wall -Werror

TARGETS = server client

SRC = ./src
BIN = ./bin

all:	$(TARGETS)

server: $(SRC)/server.o $(SRC)/transfiles.o
	gcc $(SRC)/server.o $(SRC)/transfiles.o -o $(BIN)/server

client: $(SRC)/client.o $(SRC)/transfiles.o
	gcc $(SRC)/client.o $(SRC)/transfiles.o -o $(BIN)/client

clean:
	rm -f $(TARGETS) core* *.o
