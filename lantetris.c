#include <ncurses.h>
#include <uv.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "protocol.h"

#define TIME 1000 / (level + 2)

#define NUMROWS 20
#define NUMCOLS 10

#define BLACK 8
#define RED COLOR_RED
#define GREEN COLOR_GREEN
#define YELLOW COLOR_YELLOW
#define BLUE COLOR_BLUE
#define MAGENTA COLOR_MAGENTA
#define CYAN COLOR_CYAN
#define WHITE COLOR_WHITE

struct block {
	int x, y;
};

typedef struct {
	struct block blocks[4];
	int color;
	bool flipped;
	short index;
} shape_t;

struct status_t {
	int curx, cury;
	unsigned int cycleid;
	unsigned int points;
	shape_t shape;
};

enum allowed_t { ALLOWED, NOTALLOWED, BOTTOM };

static shape_t shapes[7] = {
	{{{-1, 1}, {-1, 0}, {0, 0}, {1, 0}}, MAGENTA, false, 0},
	{{{-1, 0}, {0, 0}, {1, 0}, {2, 0}}, RED, false, 1},
	{{{1, 1}, {1, 0}, {0, 0}, {-1, 0}}, WHITE, false, 2},
	{{{1, -1}, {0, -1}, {0, 0}, {-1, 0}}, GREEN, false, 3},
	{{{1, 0}, {0, 0}, {0, -1}, {-1, -1}}, CYAN, false, 4},
	{{{-1, -1}, {0, -1}, {-1, 0}, {0, 0}}, BLUE, false, 5},
	{{{-1, 0}, {0, 0}, {1, 0}, {0, 1}}, YELLOW, false, 6},
};

static int board[NUMROWS][NUMCOLS];
static short level = 1;
static bool gamestarted = false;

uv_loop_t * loop;
uv_timer_t mv_down_timer, input_timer;

void onconn(uv_connect_t * req, int status);
void onregis(uv_write_t * req, int status);  
void parsestart(uv_stream_t * server, ssize_t nread, const uv_buf_t * buf);
void allocbuf(uv_handle_t * handle, size_t suggestsize, uv_buf_t * buf) {
	buf->base = (char *)malloc(suggestsize);
	buf->len = suggestsize;
}
void drawboard();
enum allowed_t allowed(int x, int y, shape_t * shape);
void eraseshape(int x, int y, shape_t * shape);
void drawshape(int x, int y, shape_t * shape);
void move_down(uv_timer_t * handle);
void input(uv_timer_t * handle);
static void real_rotate(shape_t * shape, bool clockwise);
static void rotate(shape_t * shape);
void clearline(int line);

char * name;
size_t namesz;
struct status_t status;

int main(int argc, char * argv[]) {
	loop = uv_default_loop();
	uv_timer_init(loop, &mv_down_timer);
	uv_timer_init(loop, &input_timer);

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
		if (msg.opcode == START && !gamestarted) {
			gamestarted = true;
			fprintf(stderr, "start %d %d", msg.seed, msg.level);
			srand(msg.seed);
			level = msg.level;
			status.curx = 4; status.cury = 1;
			status.cycleid = 0; status.points = 0;
			status.shape = shapes[rand()%7];
			loop->data = (void *)&status;
			initscr();
			start_color();
			init_pair(RED, COLOR_BLACK, RED);
			init_pair(GREEN, COLOR_BLACK, GREEN);
			init_pair(YELLOW, COLOR_BLACK, YELLOW);
			init_pair(BLUE, COLOR_BLACK, BLUE);
			init_pair(MAGENTA, COLOR_BLACK, MAGENTA);
			init_pair(CYAN, COLOR_BLACK, CYAN);
			init_pair(WHITE, COLOR_BLACK, WHITE);
			init_pair(BLACK, COLOR_WHITE, COLOR_BLACK);
			curs_set(0);
			nodelay(stdscr, TRUE);
			noecho();
			keypad(stdscr, TRUE);
			move(0, 0);
			for (int i = 0; i < NUMROWS; i++) {
                		for (int j = 0; j < NUMCOLS; j++) {
                        		board[i][j] = BLACK;
                		}
        		}
			drawshape(status.curx, status.cury, &(status.shape));
			drawboard();
			refresh();
			uv_timer_start(&mv_down_timer, move_down, TIME, TIME);
			uv_timer_start(&input_timer, input, 5, 5);
			}
	}
	else return;
}

void drawboard() {
	struct status_t * data = (struct status_t *) loop->data;
	color_set(BLACK, 0);
	mvprintw(0, 21, "%d\n", data->points);
	move(0, 0);
	for (int i = 0; i < NUMROWS; i++) {
		for (int j = 0; j < NUMCOLS; j++) {
			color_set(board[i][j], 0);
			addstr(". ");
		}
		move(i+1, 0);
	}
	move(0, 0);
}

enum allowed_t allowed(int x, int y, shape_t * shape) {
	for (int i = 0; i < 4; i++) {
		bool inrange = y + shape->blocks[i].y <= NUMROWS && y + shape->blocks[i].y >= 0
		//it is okay to do y + block.y >= NUMROWS, as this is needed and handled in the bottom case
			&& x + shape->blocks[i].x < NUMCOLS && x + shape->blocks[i].x >= 0;
		if (inrange &&
			(y + shape->blocks[i].y == NUMROWS || board[y + shape->blocks[i].y][x + shape->blocks[i].x] != BLACK )) return BOTTOM;
		else if (!inrange) return NOTALLOWED;
	}
	return ALLOWED;
}

void eraseshape(int x, int y, shape_t * shape) {
	for (int i = 0; i < 4; i++) {
		board[y + shape->blocks[i].y][x + shape->blocks[i].x] = BLACK;
	}
}

void move_down(uv_timer_t * handle) {
	struct status_t * data = (struct status_t *) loop->data;
	eraseshape(data->curx, data->cury, &(data->shape));
	enum allowed_t allwd = allowed(data->curx, data->cury+1, &(data->shape));
	if (allwd == ALLOWED) {
		data->cury++;
		drawshape(data->curx, data->cury, &(data->shape));
		drawboard();
		refresh();
	} else if (allwd == BOTTOM) {
		drawshape(data->curx, data->cury, &(data->shape));
		bool nobrick;
		int clearedlines = 0;
		for (int i = NUMROWS-1; i >= 0; i--) {
			nobrick = false;
			for (int j = 0; j < NUMCOLS; j++) {
				if (board[i][j] == BLACK) {
					nobrick = true;
					break;
				}
			}
			if (!nobrick) {
				clearline(i);
				clearedlines++;
				i++;
			}
		}
		data->points += level*10*(clearedlines >= 1 ? pow(2, clearedlines-1) : 0);
		data->curx = 4;
		data->cury = 1;
		data->cycleid++;
		data->shape = shapes[rand()%7];
		if (allowed(4, 1, &(data->shape)) == ALLOWED) {
			drawshape(4, 1, &(data->shape));
		} else {
			uv_stop(loop);
		}
		drawboard();
		refresh();
	} else return;
}

void input(uv_timer_t * handle) {
	struct status_t * data = (struct status_t *)loop->data;
	int c = getch();
	if (c != ERR) {
		switch (c) {
			case 'k':
				eraseshape(data->curx, data->cury, &(data->shape));
				shape_t tmp = data->shape;
				rotate(&(data->shape));
				if (allowed(data->curx, data->cury, &(data->shape)) == NOTALLOWED) data->shape = tmp;
				drawshape(data->curx, data->cury, &(data->shape));
				drawboard();
				refresh();
				break;
			case 'j':
				eraseshape(data->curx, data->cury, &(data->shape));
				if (allowed(data->curx - 1, data->cury, &(data->shape)) == ALLOWED) data->curx--;
				drawshape(data->curx, data->cury, &(data->shape));
				drawboard();
				refresh();
				break;
			case 'l':
				eraseshape(data->curx, data->cury, &(data->shape));
                                if (allowed(data->curx + 1, data->cury, &(data->shape)) == ALLOWED) data->curx++;
				drawshape(data->curx, data->cury, &(data->shape));
                                drawboard();
                                refresh();
                                break;
			case ' ':
				unsigned int cycid = data->cycleid;
				move_down(NULL);
				while (data->cycleid == cycid) move_down(NULL);
				break;
			case 'q':
				endwin();
				uv_stop(loop);
			default: break;
		}
	}
}

// This rotation code (including the coordinate system) is copied/inspired from Abraham vd Merwe <abz@blio.net>
// fantastic tetris implementation tint (github.com/DavidGriffith/tint) (check
// it out). I mainly adopted it to my (in my opionion more intuitive coordinate system starting a 0 and not -1.
// Anyway thankyou from me for that source of inspiration.
static void real_rotate(shape_t * shape, bool clockwise) {
	if (clockwise) {
		for (int i = 0; i < 4; i++) {
			int tmp = shape->blocks[i].x;
			shape->blocks[i].x = -shape->blocks[i].y;
			shape->blocks[i].y = tmp;
		}
	} else {
		for (int i = 0; i < 4; i++) {
			int tmp = shape->blocks[i].x;
			shape->blocks[i].x = shape->blocks[i].y;
			shape->blocks[i].y = -tmp;
		}
	}
}

static void rotate(shape_t * shape) {
	switch (shape->index) {
		case 6:
		case 0:
		case 2:
			real_rotate(shape, false);
			break;
		case 5:
			break;
		case 4:
			if (shape->flipped) real_rotate(shape, true); else real_rotate(shape, false);
			shape->flipped = !shape->flipped;
			break;
		case 3:
		case 1:
			if (shape->flipped) real_rotate(shape, false); else real_rotate(shape, true);
			shape->flipped = !shape->flipped;
			break;
	}
}

void drawshape(int x, int y, shape_t * shape) {
	for (int i = 0; i < 4; i++) {
                board[y + shape->blocks[i].y][x + shape->blocks[i].x] = shape->color;
        }
}

void clearline(int line) {
	while (line > 0) {
		for (int j = 0; j < NUMCOLS; j++) {
			board[line][j] = board[line-1][j];
		}
		line--;
	}
	for (int j = 0; j < NUMCOLS; j++) {
		board[0][j] = BLACK;
	}
}
