CLIENT=ltetris-clt
CC=cc
LINK= -lncurses -luv

all: testclt

protocol.o: protocol.c
	$(CC) -c protocol.c

protocoltest: test_protocol.c protocol.o
	$(CC) -o prototest test_protocol.c protocol.o

test: protocoltest
	./prototest

testclt: lantetris.c protocol.o
	$(CC) -o testclt lantetris.c protocol.o $(LINK)
clean:
	rm -rf $(CLIENT)
	rm -rf protocol.o
	rm -rf prototest
	rm -rf testclt

