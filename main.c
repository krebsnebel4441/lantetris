#include <ncurses.h>
#include <uv.h>

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

struct status {
	int curx, cury;
	short shape;
};

struct block {
	int x, y;
};

typedef struct {
	struct block blocks[4];
	int color;
} shape_t;

static shape_t shapes[7] = {
	{{{0, 0}, {1, 0}, {2, 0}, {0, 1}}, MAGENTA},
	{{{0, 0}, {1, 0}, {2, 0}, {3, 0}}, RED},
	{{{0, 0}, {1, 0}, {2, 0}, {2, 1}}, WHITE},
	{{{1, 0}, {2, 0}, {0, 1}, {1, 1}}, GREEN},
	{{{0, 0}, {1, 0}, {1, 1}, {2, 1}}, CYAN},
	{{{0, 0}, {1, 0}, {0, 1}, {1, 1}}, BLUE},
	{{{0, 0}, {1, 0}, {2, 0}, {1, 1}}, YELLOW},
};

static int board[NUMROWS][NUMCOLS];
static short level = 1;

void drawboard();
bool allowed(int, int, short);
void eraseshape(int, int, short);
void move_down(uv_timer_t *);

uv_loop_t * loop;

int main() {
	struct status status = {3, 0, 0};

	uv_timer_t timer;

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
	move(0, 0);
	for (int i = 0; i < NUMROWS; i++) {
		for (int j = 0; j < NUMCOLS; j++) {
			board[i][j] = BLACK;
		}
	}
	for (int i = 0; i < 4; i++) {
		board[status.cury + shapes[status.shape].blocks[i].y][status.curx + shapes[status.shape].blocks[i].x] = shapes[status.shape].color; 
	}
	drawboard();
	refresh();
	
	loop = uv_default_loop();
	uv_loop_init(loop);
	uv_timer_init(loop, &timer);
	timer.data = (void *) &status;
	uv_timer_start(&timer, move_down, TIME, TIME);
	uv_run(loop, UV_RUN_DEFAULT);
	
	uv_loop_close(loop);
	getch();
	endwin();
	return 0;
}

void drawboard() {
	for (int i = 0; i < NUMROWS; i++) {
		for (int j = 0; j < NUMCOLS; j++) {
			color_set(board[i][j], 0);
			addstr(". ");
		}
		move(i+1, 0);
	}
	move(0, 0);
}

bool allowed(int x, int y, short shape) {
	for (int i = 0; i < 4; i++) {
		if (x + shapes[shape].blocks[i].x >= NUMCOLS
		 || y + shapes[shape].blocks[i].y >= NUMROWS) {
		 return false;
		}
	}
	return true;
}

void eraseshape(int x, int y, short shape) {
	for (int i = 0; i < 4; i++) {
		board[y + shapes[shape].blocks[i].y][x + shapes[shape].blocks[i].x] = BLACK;
	}
}

void move_down(uv_timer_t * handle) {
	struct status * data = (struct status *) handle->data;
	if (allowed(data->curx, data->cury+1, data->shape)) {
		eraseshape(data->curx, data->cury, data->shape);
		data->cury++;
		for (int i = 0; i < 4; i++) {
			board[data->cury + shapes[data->shape].blocks[i].y][data->curx + shapes[data->shape].blocks[i].x] = shapes[data->shape].color;
		}
		drawboard();
		refresh();
	} else {
		uv_timer_stop(handle);
		uv_stop(loop);
	}
}
