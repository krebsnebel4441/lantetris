#include <ncurses.h>
#include <uv.h>
#include <stdlib.h>
#include <string.h>
#include "protocol.h"

#define TIME 1000 / (level + 2)

#define NUMROWS 20
#define NUMCOLS 10

#define BLACK 8
#define RED COLOR_RED
#define GREEN COLOR_GREEN
#define YELLOW COLOR_YELLOW
#define BLUE COLOR_BLUE
#define MANGENTA COLOR_MAGENTA
#define CYAN COLOR_CYAN
#define WHITE COLOR_WHITE

uv_loop_t * loop;

void onconn(uv_connect_t * req, int status);
void onregis(uv_write_t * req, int status);  
void parsestart(uv_stream_t * server, ssize_t nread, const uv_buf_t * buf);
void allocbuf(uv_handle_t * handle, size_t suggestsize, uv_buf_t * buf) {
	buf->base = (char *)malloc(suggestsize);
	buf->len = suggestsize;
}

char * name;
size_t namesz;

int main(int argc, char * argv[]) {
	loop = uv_default_loop();
	struct sockaddr_in server;
	if (argc < 3) {
		fprintf(stderr, "invaled arguments\n");
		return -1;
	}
	name = malloc(strlen(argv[2]));
	memcpy(name, argv[2], strlen(argv[2]));
	namesz = strlen(argv[2]);
	if (uv_ip4_addr(argv[1], 7000, &server) != 0) {
		fprintf(stderr, "bad address\n");
		return -1;
	}
	uv_tcp_t socket;
	uv_tcp_init(loop, &socket);
	uv_connect_t * connect = (uv_connect_t *)malloc(sizeof(uv_connect_t));
	uv_tcp_connect(connect, &socket, (const struct sockaddr *)&server, onconn);
	return uv_run(loop, UV_RUN_DEFAULT);
}

void onconn(uv_connect_t * req, int status) {
	if (status < 0) {
		fprintf(stderr, "Failed to connect to server, maybe check the address\n");
		exit(-1);
	}
	message_t join;
	join.opcode = JOIN;
	join.strlen = namesz; 
	join.name = malloc(namesz);
	memcpy(join.name, name, join.strlen);
	bytemsg_t msg = encode_message(&join);
	uv_write_t * write = malloc(sizeof(uv_write_t));
	uv_buf_t buf = uv_buf_init(msg.buf, msg.size);
	uv_write(write, (uv_stream_t *)req->handle, &buf, 1, onregis);
}

void onregis(uv_write_t * req, int status) {
	if (status < 0) {
		fprintf(stderr, "Failed to register\n");
	}
	uv_read_start((uv_stream_t *) req->handle, allocbuf, parsestart);
}

void parsestart(uv_stream_t * server, ssize_t nread, const uv_buf_t * buf) {
	if (nread >= 6) {
		message_t msg = decode_message(buf->base, buf->len);
		if (msg.opcode == START) {
			fprintf(stderr, "start");
		}
	}
	else return;

}
