#ifndef _LINE_EDIT_H_
#define _LINE_EDIT_H_

#define LINE_MAX 2048
#define CHAR_MAX 512
#define MAX_PASSWORD 25
#include "terminal.h"

typedef struct append_buffer_t {
    char *buffer;
    size_t length;
} AppendBuffer;

#define abInit() {NULL, 0}
void abAppend(AppendBuffer *ab, const char *buffer, size_t buflen);

char *editLine(const char *prompt, const char *initial);
char *editPassword(const char *prompt);

#endif 