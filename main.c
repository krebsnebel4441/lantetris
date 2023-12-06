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

struct block {
	int x, y;
};

// The blocks of the shape are reside in a coordinate system like this
//   -1 0 1 2
//-1 
// 0
// 1
// The x and the y value of a block specifies it's coordinate in that system.
// These coordinate systems are later transformed into the 10x20 coordinate system of the board
typedef struct {
	struct block blocks[4];
	int color;
	bool flipped;
	short index;
} shape_t;

struct status {
	int curx, cury;
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

void drawboard();
enum allowed_t allowed(int, int, shape_t *);
void eraseshape(int, int, shape_t *);
void drawshape(int, int, shape_t *);
void move_down(uv_timer_t *);
void input(uv_idle_t *);
static void real_rotate(shape_t *, bool);
static void rotate(shape_t *);

uv_loop_t * loop;

int main() {
	struct status status = {4, 1, shapes[6]};

	uv_timer_t timer;
	uv_idle_t idler;
	
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
	move(0, 0);
	
	for (int i = 0; i < NUMROWS; i++) {
		for (int j = 0; j < NUMCOLS; j++) {
			board[i][j] = BLACK;
		}
	}
	drawshape(status.curx, status.cury, &(status.shape));
	drawboard();
	refresh();
	
	loop = uv_default_loop();
	uv_loop_init(loop);
	loop->data = (void *) &status;
	
	uv_timer_init(loop, &timer);
	uv_timer_start(&timer, move_down, TIME, TIME);
	
	uv_idle_init(loop, &idler);
	uv_idle_start(&idler, input);

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
	struct status * data = (struct status *) loop->data;
	eraseshape(data->curx, data->cury, &(data->shape));
	enum allowed_t allwd = allowed(data->curx, data->cury+1, &(data->shape));
	if (allwd == ALLOWED) {
		data->cury++;
		drawshape(data->curx, data->cury, &(data->shape));
		drawboard();
		refresh();
	} else if (allwd == BOTTOM) {
		drawshape(data->curx, data->cury, &(data->shape));
		data->curx = 4;
		data->cury = 1;
		data->shape = shapes[(data->shape.index + 1)%7];
		drawshape(4, 1, &(data->shape));
		drawboard();
		refresh();
	} else return;	
}

void input(uv_idle_t * handle) {
	struct status * data = (struct status *)loop->data;
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
			case 'q':
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
