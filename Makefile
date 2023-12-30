CLIENT=ltetris-clt
CC=cc
LINK= -lm -lncurses -luv

all: client server

protocol.o: protocol.c
	$(CC) -c protocol.c

protocoltest: test_protocol.c protocol.o
	$(CC) -o prototest test_protocol.c protocol.o

test: protocoltest
	./prototest

client: lantetris.c protocol.o
	$(CC) -o client lantetris.c protocol.o $(LINK)

server: server.c protocol.o
	$(CC) -o server server.c protocol.o $(LINK)

clean:
	rm -rf $(CLIENT)
	rm -rf protocol.o
	rm -rf prototest
	rm -rf client 
	rm -rf server

