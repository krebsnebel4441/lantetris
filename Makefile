CLIENT=ltetris-clt
CC=cc
LINK= -lncurses -luv

all: client

client:	main.c
	$(CC) -o $(CLIENT) main.c $(LINK)

clean:
	rm -rf $(CLIENT)

