#include <uv.h>
#include <stdlib.h>
#include <stdio.h>
#include "protocol.h"

#define DEFAULT_BACKLOG 128

struct client {
	size_t strlen;
	char * name;
	unsigned int score;
};

typedef struct {
	size_t capacity, size;
	uv_tcp_t ** connections;
} clients_t;


clients_t initclients() {
	clients_t new;
	new.size = 0;
	new.capacity = 5;
	new.connections = malloc(sizeof(uv_tcp_t *) * 5);
	return new;
}

void insertclient(clients_t * arr, struct client * newclt, uv_tcp_t * conn) {
	if (arr->size > arr->capacity) {
		arr->connections = realloc(arr->connections, sizeof(uv_tcp_t *)*(arr->capacity + 5));
		arr->capacity += 5;
	}
	conn->data = (void *)newclt;
	arr->connections[arr->size] = conn;
	arr->size++;
}


static enum { REGISTRATION, GAME } gamestate = REGISTRATION;
static char * startmsg;
static int runningclients = 0;

void onnewconn(uv_stream_t * server, int status);
void alloc_buffer(uv_handle_t * handle, size_t suggested_size, uv_buf_t * buf);
void parse(uv_stream_t * client, ssize_t nread, const uv_buf_t * buf);
void onstartwrite(uv_write_t *req, int status);
void startgame(uv_timer_t * handle);
void onstartsent(uv_write_t * req, int status);

uv_loop_t * loop;
uv_timer_t start;

int main() {
	clients_t clients = initclients();
	struct sockaddr_in addr;
	message_t msg;
	msg.opcode = START;
	msg.seed = time(NULL);
	msg.level = 5;
	bytemsg_t b = encode_message(&msg);
	startmsg = b.buf;

	loop = uv_default_loop();

	loop->data = (void *)&clients;
	
	uv_timer_init(loop, &start);
	uv_timer_start(&start, startgame, 5000, 0);

    	uv_tcp_t server;
    	uv_tcp_init(loop, &server);

    	uv_ip4_addr("0.0.0.0", 7003, &addr);

    	uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
    	int r = uv_listen((uv_stream_t*) &server, DEFAULT_BACKLOG, onnewconn);
    	if (r) {
        	fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        	return 1;
    	}
    	uv_run(loop, UV_RUN_DEFAULT);
	struct client * biggest = NULL;
	int greatestscore = 0;
	for (int i = 0; i < clients.size; i++) {
		struct client * show = (struct client *)clients.connections[i]->data;
		printf("%d, %s reached %d points\n", i+1, show->name, show->score);
		if (show->score > greatestscore) {
			greatestscore = show->score;
			biggest = show;
		}
	}
	if (biggest != NULL) printf("%s, won\n", biggest->name);
	return 0;
}

void onnewconn(uv_stream_t * server, int status) {
	if (status < 0) {
        	fprintf(stderr, "New conn error %s\n", uv_strerror(status));
        	return;
    	}
	uv_tcp_t * client = malloc(sizeof(uv_tcp_t));
    	uv_tcp_init(loop, client);
    	if (uv_accept(server, (uv_stream_t *)client) == 0) {
        	uv_read_start((uv_stream_t*)client, alloc_buffer, parse);
    	}
}

void parse(uv_stream_t * client, ssize_t nread, const uv_buf_t * buf) {
	if (nread > 0) {
		clients_t * clients = (clients_t *)loop->data;
		message_t msg = decode_message(buf->base, buf->len);
		if (msg.opcode == JOIN && gamestate == REGISTRATION) {
			struct client * newclient = malloc(sizeof(struct client));
			newclient->strlen = msg.strlen;
			newclient->name = msg.name;
			newclient->score = 0;
			insertclient(clients, newclient, (uv_tcp_t *)client);
			runningclients++;
			printf("client with name %s registered\n", msg.name);
		} else if (msg.opcode == GAMEOVER) {
			runningclients--;
			if (runningclients <= 0) {
				uv_stop(loop);
			}
		} else if (msg.opcode == STATUS) {
			struct client * change = (struct client *)client->data;
			change->score += msg.score;
		}
	}
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
	buf->base = (char*) malloc(suggested_size);
    	buf->len = suggested_size;
}

void onstartwrite(uv_write_t *req, int status) {
	if (status < 0) {
		fprintf(stderr, "Failed to write start message");
	}
	free(req);
}

void startgame(uv_timer_t * handle) {
	clients_t * data = (clients_t *)loop->data;
	for (int i = 0; i < data->size; i++) {
		uv_write_t * write = malloc(sizeof(uv_write_t));
		uv_buf_t buf = uv_buf_init(startmsg, 6);
		uv_write(write, (uv_stream_t *)data->connections[i], &buf, 1, onstartsent);
	}
}

void onstartsent(uv_write_t * req, int status) {
	if (status < 0) {
		fprintf(stderr, "failed to sent start");
	}
	free(req);
}
