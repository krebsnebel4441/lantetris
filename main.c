#include <ncurses.h>

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

void drawboard();
bool allowed(int, int, short);
void eraseshape(int, int, short);

int main() {
	int curx = 3, cury = 0;

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
	#define S 6 
	for (int i = 0; i < 4; i++) {
		board[cury + shapes[S].blocks[i].y][curx + shapes[S].blocks[i].x] = shapes[S].color;
	}
	drawboard();
	refresh();
	while (allowed(curx, cury+1, S)) {
		eraseshape(curx, cury, S);
		cury++;
		for (int i = 0; i < 4; i++) {
			board[cury + shapes[S].blocks[i].y][curx + shapes[S].blocks[i].x] = shapes[S].color;
		}
		getch();
		drawboard();
		refresh();
	};
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
		board[y + shapes[S].blocks[i].y][x + shapes[S].blocks[i].x] = BLACK;
	}
}
