#include "editline.h"

/* Note that Chinese characters occupy 2 block of space in most terminal. */
typedef struct line_state_t{
    char buffer[LINE_MAX];  /* Edited line buffer. */
    size_t buffer_length;   /* Edited line buffer length. */
    ssize_t lcc;            /* Count of line characters in terminal. */
    char *lc[CHAR_MAX];     /* Line characters in terminal. */
    char *prompt;           /* Prompt to display. */
    size_t prompt_length;   /* Prompt length. */
    ssize_t pcc;            /* Count of prompt characters. */
    ssize_t pcs;            /* Actucal display size of prompt. */
    size_t cursor;          /* The position of cursor in the terminal. */
    size_t columns;         /* Number of columns in terminal. */
    int passwordfield;      /* Indicate if this is a password field. */
} LineState;

typedef enum {
    CharVisible,
    UTF8_2BHead,
    UTF8_3BHead,
    UTF8_4BHead,
    UTF8_5BHead,
    UTF8_6BHead,
    UTF8_Body,
    CharFunctional,
    CharUnrecognize
} CharType;

typedef enum {
    WrongPrompt,
    WrongLine,
    Good
} ErrType;

int errno;
struct termios original_termios;
int atexit_registered = 0;

void abAppend(AppendBuffer *ab, const char *buffer, size_t buflen)
{
    char *new_buffer = realloc(ab->buffer, ab->length + buflen);
    if (!new_buffer) return;
    memcpy(new_buffer + ab->length, buffer, buflen);
    ab->buffer = new_buffer;
    ab->length += buflen;
}

/* [begin, end) */
ssize_t calculateColumn(LineState *state, ssize_t begin, ssize_t end)
{
    if (end > state->lcc || begin < 0 || end <= begin) {
        return 0;
    }
    int high_address;
    if (end == state->lcc) {
        high_address = (int)(state->lc[end - 1] + strlen(state->lc[end - 1]));
    } else {
        high_address = (int)state->lc[end];
    }
    int low_address = (int)state->lc[begin];
    int m_usage = high_address - low_address;
    ssize_t total = (ssize_t)end - begin;
    ssize_t x = (m_usage - total) / 2;
    ssize_t y = total - x;
    return 2 * x + y;
}

/* I mean seriously, this needs to be optimized! */
void refreshLineNormal(LineState *state)
{
    ssize_t psize = state->pcs;
    size_t columns = state->columns;
    size_t origin = 0;
    ssize_t cursor = state->cursor;
    ssize_t bsize = calculateColumn(state, origin, cursor);
    if (bsize < 0) return;
    while (psize + bsize >= columns) {
        origin++;
        bsize = calculateColumn(state, origin, cursor);
    }
    cursor = bsize;
    ssize_t count = state->lcc - origin;

    bsize = calculateColumn(state, origin, origin + count);
    while (psize + bsize >= columns) {
        --count;
        bsize = calculateColumn(state, origin, origin + count);
    }
    size_t len = 0;
    char *buffer = state->lc[origin];
    if (origin + count == state->lcc) len = strlen(buffer);
    else len = (size_t)(state->lc[origin + count] - state->lc[origin]);

    char seq[64];
    AppendBuffer ab = abInit();

    snprintf(seq, 64, "\r");
    abAppend(&ab, seq, strlen(seq));

    abAppend(&ab, state->prompt, state->prompt_length);
    abAppend(&ab, buffer, len);

    snprintf(seq, 64, "\x1b[0K");
    abAppend(&ab, seq, strlen(seq));

    snprintf(seq, 64, "\r\x1b[%ldC", cursor + psize);
    abAppend(&ab, seq, strlen(seq));
    write(STDOUT_FILENO, ab.buffer, ab.length);
    free(ab.buffer);
}

void refreshLinePassword(LineState *state)
{
    static char *password_buffer = "*************************";

    if (state->lcc >= MAX_PASSWORD) {
        state->buffer[state->lcc] = '\0';
        state->lcc = MAX_PASSWORD;
    }

    char *prompt = state->prompt;
    ssize_t psize = state->pcs;
    ssize_t bsize = state->lcc;
    size_t columns = state->columns;
    size_t cursor = state->cursor;
    size_t origin = 0;
    while (psize + cursor >= columns) {
        origin++;
        cursor--;
        bsize--;
    }
    while (psize + bsize >= columns) {
        bsize--;
    }

    char *buffer = password_buffer + origin;

    char seq[64];
    AppendBuffer ab = abInit();
    snprintf(seq, 64, "\r");
    abAppend(&ab, seq, strlen(seq));

    abAppend(&ab, prompt, strlen(prompt));
    abAppend(&ab, buffer, (size_t)bsize);

    snprintf(seq, 64, "\x1b[0K");
    abAppend(&ab, seq, strlen(seq));

    snprintf(seq, 64, "\r\x1b[%zuC", cursor + psize);
    abAppend(&ab, seq, strlen(seq));
    if (write(STDOUT_FILENO, ab.buffer, ab.length) < 0) {
        /* Do nothing since there's no way recovering from error. */
    }
    free(ab.buffer);
}

void refreshLine(LineState *state)
{
    if (state->passwordfield) {
        refreshLinePassword(state);
    } else {
        refreshLineNormal(state);
    }
}

CharType charTell(char ch)
{
    unsigned char c = (unsigned char)ch;
    if ((ch > 0 && ch < 32) || ch == 127) {
        return CharFunctional;
    } else if (ch >= 32 && ch <= 126) {
        return CharVisible;
    } else if (c >= (1 << 7) && c < (3 << 6)) {
        return UTF8_Body;
    } else if (c >= (3 << 6) && c < (7 << 5)) {
        return UTF8_2BHead;
    } else if (c >= (7 << 5) && c < (15 << 4)) {
        return UTF8_3BHead;
    } else if (c >= (15 << 4) && c < (31 << 3)) {
        return UTF8_4BHead;
    } else if (c >= (31 << 3) && c < (63 << 2)) {
        return UTF8_5BHead;
    } else if (c >= (63 << 2) && c < (127 << 1)) {
        return UTF8_6BHead;
    }
    return CharUnrecognize;
}

void clearLineState(LineState *state)
{
    free(state->prompt);
    state->prompt = NULL;
    state->prompt_length = 0;
    state->pcc = 0;
    for (int i = 0; i < CHAR_MAX; i++) {
        state->lc[i] = NULL;
    }
    state->buffer[0] = '\0';
    state->buffer_length = 0;
    state->lcc = 0;
    state->cursor = 0;
}

ssize_t stringTell(char *buffer, char *bcaddrs[], size_t len, size_t bclen)
{
    size_t ret = 0;
    for (int i = 0; i < len; i++) {
        CharType t = charTell(buffer[i]);
        if (t == CharVisible || t == UTF8_2BHead ||
            t == UTF8_3BHead || t == UTF8_4BHead ||
            t == UTF8_5BHead || t == UTF8_6BHead) {
            ret++;
            if (bcaddrs && ret <= bclen) {
                bcaddrs[ret - 1] = buffer + i;
            }
        } else if (t != UTF8_Body) {
            return -1;
        }
    }
    return ret;
}

ssize_t stringTell2(char *buffer, size_t *len, ssize_t *count)
{
    size_t i;
    ssize_t blocks = 0;
    ssize_t chcount = 0;
    for (i = 0; buffer[i] != '\0'; i++) {
        CharType t = charTell(buffer[i]);
        if (t == CharVisible) {
            blocks++;
            chcount++;
        } else if (t == UTF8_3BHead) {
            blocks += 2;
            chcount++;
        } else if (t != UTF8_Body) {
            printf("something wrong : %zu\n", i);
            printf("it is: %s\n", buffer);
            printf("length is: %zu\n", strlen(buffer));
            return -1;
        }
    }
    *len = i;
    *count = chcount;
    return blocks;
}

int getLineState(const char *prompt, const char *buffer, LineState *state)
{
    updatewindowsize();
    state->columns = (size_t)getwidth();

    if (!buffer || buffer[0] == '\0') {
        state->buffer_length = 0;
        state->buffer[0] = '\0';
        state->lcc = 0;
        state->cursor = 0;
        state->lc[0] = state->buffer;   
    } else {
        sprintf(state->buffer, "%s", buffer);
        state->buffer_length = strlen(buffer);
        state->lcc = stringTell(state->buffer, state->lc, state->buffer_length, CHAR_MAX);
        state->cursor = (size_t)state->lcc;
    }

    if (prompt) {
        state->prompt = strdup(prompt);
        state->pcs = stringTell2(state->prompt, &state->prompt_length, &state->pcc);
    } else {
        state->prompt = strdup("");
        state->prompt_length = 0;
        state->pcc = 0;
        state->pcs = 0;
    }
    if (state->pcs < 0 || state->lcc < 0) {
        clearLineState(state);
        return -1;
    }
    return 0;
}

void exitRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

/* Raw mode: 1960 magic shit. */
void enterRawMode()
{
    tcgetattr(STDIN_FILENO, &original_termios);
    if (!atexit_registered) {
        atexit(exitRawMode);
        atexit_registered = 1;
    }
    struct termios raw = original_termios;
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes - disable post processing */
    raw.c_oflag &= ~(OPOST);
    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes - choing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer.
     * We want read to return every single byte, without timeout. */
    raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void lineInsert(LineState *state, char *src)
{
    size_t length = strlen(src);
    /* Move state->buffer. */
    if (state->buffer_length + length >= LINE_MAX) return;
    if (state->lcc + 1 > CHAR_MAX) return;
    if (state->passwordfield && state->buffer_length >= MAX_PASSWORD) return;

    int cursor = state->cursor;
    int cursor_pos;
    if (cursor < state->lcc) {
        cursor_pos = (int)(state->lc[cursor] - state->buffer);
    } else {
        cursor_pos = state->buffer_length;
        state->lc[cursor] = state->buffer + state->buffer_length;
    }
    for (int i = state->buffer_length + length; i >= cursor_pos + length; i--) {
        state->buffer[i] = state->buffer[i - length];
    }
    for (int i = 0; i < length; i++) {
        state->buffer[cursor_pos + i] = src[i];
    }

    /* Move buffer addresses. */
    state->lc[0] = state->buffer;
    for (int i = state->lcc; i > cursor && i > 0; i--) {
        state->lc[i] = state->lc[i - 1];
        state->lc[i] += length;
    }
    state->buffer_length += length;
    state->buffer[state->buffer_length] = '\0';
    state->cursor++;
    state->lcc++;
}

void lineEditDelete(LineState *state)
{
    int cursor = state->cursor;
    int charsize;
    if (cursor == state->lcc) {
        cursor--;
        charsize = (int)(strlen(state->lc[cursor]));
    } else if (cursor <= 0 || cursor > state->lcc) {
        state->cursor = 0;
        return;
    } else {
        cursor--;
        int cursor_pos = (int)(state->lc[cursor] - state->buffer);
        charsize = (int)(state->lc[cursor + 1] - state->lc[cursor]);
        for (int i = cursor_pos; i < state->buffer_length - charsize; i++) {
            state->buffer[i] = state->buffer[i + charsize];
        }
    }
    state->buffer_length -= charsize;
    state->buffer[state->buffer_length] = '\0';
    state->cursor--;
    state->lcc--;
}

char *_lineEdit(LineState *state)
{
    enterRawMode();
    refreshLine(state);
    char buffer[4];
    int nread;
    char *ret = NULL;
    while (1) {
        /* Catch 4 mode: enter, backspace, ctrl+c, characters. */
        nread = read(STDIN_FILENO, buffer, 1);
        if (nread <= 0) {
            clearLineState(state);
            printf("nread < 0 error\n");
            break;
        }
        if (buffer[0] == KeyActionRawEnter) {
            ret = strdup(state->buffer);
            state->cursor = state->lcc;
            refreshLine(state);
            clearLineState(state);
            break;
        } else if (charTell(buffer[0]) == CharVisible) {
            buffer[1] = '\0';
            lineInsert(state, buffer);
        } else if (charTell(buffer[0]) == UTF8_3BHead) {
            if (read(STDIN_FILENO, buffer + 1, 1) == -1) break;
            if (read(STDIN_FILENO, buffer + 2, 1) == -1) break;
            buffer[3] = '\0';
            lineInsert(state, buffer);
        } else if (buffer[0] == KeyActionEscape) {
            if (read(STDIN_FILENO, buffer, 1) == -1) break;
            if (read(STDIN_FILENO, buffer + 1, 1) == -1) break;
            if (buffer[0] != '[') continue;
            if (buffer[1] == 'C') {
                if (state->cursor < state->lcc) state->cursor++;
            } else if (buffer[1] == 'D') {
                if (state->cursor > 0) state->cursor--;
            } else continue;
        } else if (buffer[0] == KeyActionBackspace) {
            if (state->cursor <= 0) continue;
            lineEditDelete(state);
        } else if (buffer[0] == ControlC) {
            break;
        }
        refreshLine(state);
    }
    exitRawMode();
    return ret;
}

char *editLine(const char *prompt, const char *initial)
{
    LineState state;
    int result = getLineState(prompt, initial, &state);
    if (result < 0) {
        printf("get line state error!\n");
        return NULL;
    }
    state.passwordfield = 0;
    return _lineEdit(&state);
}

char *editPassword(const char *prompt)
{
    LineState state;
    int result = getLineState(prompt, NULL, &state);
    if (result < 0) {
        printf("get line state error!\n");
        return NULL;
    }
    state.passwordfield = 1;
    return _lineEdit(&state);
}