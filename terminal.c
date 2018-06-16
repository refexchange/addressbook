#include "terminal.h"
#include <sys/ioctl.h>


struct winsize _frame;
int _frame_set = 0;

typedef enum {
	TerminalModeOriginal,
	TerminalModeCBreak,
	TerminalModeRegular
} TerminalMode;

struct termios originalmode;
int originalmode_set = 0;
TerminalMode currentMode = TerminalModeOriginal;

int cbreakmode()
{
	struct termios nt;
	tcgetattr(STDIN_FILENO, &nt);
	if (!originalmode_set) {
		originalmode = nt;
		atexit(resetmode);
		originalmode_set = 1;
	}
	nt.c_lflag &= ~(ECHO | ICANON);
	nt.c_cc[VMIN] = 1;
	nt.c_cc[VTIME] = 0;
	currentMode = TerminalModeCBreak;
	return tcsetattr(STDIN_FILENO, TCSAFLUSH, &nt) < 0 ? NO : YES;
}

int regularmode()
{
	struct termios nt;
	tcgetattr(STDIN_FILENO, &nt);
	if (!originalmode_set) {
		originalmode_set = 1;
		originalmode = nt;
	}
	nt.c_lflag |= (ECHO | ICANON);
	currentMode = TerminalModeRegular;
	return tcsetattr(STDIN_FILENO, TCSAFLUSH, &nt) < 0 ? NO : YES;
}

void resetmode()
{
	if (!originalmode_set) {
		return;
	}
	currentMode = TerminalModeOriginal;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalmode);
}

void mask()
{
	for (int i = 0; i < getwidth(); i++) {
		putchar(' ');
	}
}

void highlight(const char *line, int row_index)
{
	printf("\x1b[%d;1H\x1b[0K\x1b[47;30m%s\x1b[0m", row_index, line);
	printf("\x1b[%d;%dH", getheight() - 1, getwidth());
}

void dehighlight(const char *line, int row_index)
{
	printf("\x1b[0m\x1b[%d;0H\x1b[K%s", row_index, line);
	printf("\x1b[%d;%dH", getheight() - 1, getwidth());
}

void clearlines(int begin, int end)
{
	int column = getwidth();
	for (int i = begin; i <= end; i++) {
		printf("\x1b[%d;1H\x1b[0K", i);
	}
}

void clearscr()
{
	return clearlines(1, getheight());
}

int updatewindowsize()
{
	_frame_set = 1;
	return ioctl(STDIN_FILENO, TIOCGWINSZ, (char*)&_frame);
}

int getwidth()
{
	if (!_frame_set) updatewindowsize();
	return _frame.ws_col;
}

int getheight()
{
	if (!_frame_set) updatewindowsize();
	return _frame.ws_row;
}

int keyboardPress()
{
	static int a, b, c;
	a = getchar();
	if (a != KeyActionEscape) return a;
	b = getchar();
	if (b != (int)'[') return KeyActionNull;;
	c = getchar();
	if (c == 65) return KeyActionUp;
	else if (c == 66) return KeyActionDown;
	else if (c == 67) return KeyActionRight;
	else if (c == 68) return KeyActionLeft;
	else return KeyActionNull;
}

void hidecursor()
{
	printf("\x1b[?25l");
}

void showcursor()
{
	printf("\x1b[?25h");
}

void raw_debug(const char *message)
{
	updatewindowsize();
	printf("\x1b[%d;1H\x1b[0K%s", getheight(), message);
}

void err_exit(const char *message)
{
    resetmode();
    clearscr();
    printf("\x1b[1;1H%s\n", message);
    exit(0);
}