CLIENT=ltetris-clt
CC=cc
LINK= -lncurses -luv

all: client

client:	main.c
	$(CC) -o $(CLIENT) main.c $(LINK)

protocol.o: protocol.c
	$(CC) -c protocol.c

protocoltest: test_protocol.c protocol.o
	$(CC) -o prototest test_protocol.c protocol.o

test: protocoltest
	./prototest

clean:
	rm -rf $(CLIENT)
	rm -rf protocol.o
	rm -rf prototest

