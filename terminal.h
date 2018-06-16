/* Commandline Interface Tools */

#ifndef _TERMINAL_H_
#define _TERMINAL_H_

#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define YES 1
#define NO 0

typedef struct coordinate_t {
    int X;
    int Y;
} Coordinate;

typedef enum {
    KeyActionNull = 0,        /* NULL */
    ControlA = 1,             /* Move to the beginning of line. */
    ControlB = 2,             /* Move backward. */
    ControlC = 3,             /* Generate INT. */
    ControlD = 4,             /* Generate EOF. */
    ControlE = 5,             /* Move to the end of line. */
    ControlF = 6,             /* Move forward. */
    ControlK = 11,            /* Delete line content after the cursor. */
    KeyActionEnter = 10,      /* Enter */
    KeyActionRawEnter = 13,   /* Enter in raw mode. */
    KeyActionEscape = 27,     /* Escape Sequence begin. */
    KeyActionBackspace = 127, /* Deletes character. */
    KeyActionUp = 128,        /**/
    KeyActionDown = 129,      
    KeyActionLeft = 130,
    KeyActionRight = 131,
} KeyAction;


int regularmode();
int cbreakmode();
void resetmode();

void highlight(const char *line, int row_index);
void dehighlight(const char *line, int row_index);

void clearlines(int begin, int end);
void clearscr();
int getwidth();
int getheight();
int updatewindowsize();

int keyboardPress();

void hidecursor();
void showcursor();

void raw_debug(const char *message);
void err_exit(const char *message);

#endif /* _TERMINAL_H_ */