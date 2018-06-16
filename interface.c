#include "util.h"
#include "terminal.h"
#include "interface.h"
#include "line_edit.h"
#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#ifndef palloc
#define palloc(x) ((x*)calloc(1, sizeof(x)))
#endif

void dummy(void *target) {}

int recalculateHintHeight(const char *hint)
{
    if (!hint) return 0;
    int hint_height = 1;
    int col = 0;
    int maxcol = getwidth();
    for (int i = 0; hint[i] != '\0'; i++) {
        if (hint[i] == '\n' || col >= maxcol) {
            col = 0;
            hint_height++;
        }
    }
    return hint_height;
}

TableInterface *getMenuTable(int count, const char *titles[], const char *hint)
{
    if (count < 1 || !titles) return NULL;
    int hint_height = recalculateHintHeight(hint);

    TableInterface *table = palloc(TableInterface);
    table->hint = strdup(hint);
    table->context = NULL;
    table->count = count;
    table->display_origin = 0;
    table->display_bound = getheight() - hint_height - 1;
    if (table->display_bound > table->count - 1) {
        table->display_bound = table->count - 1;
    }
    table->focus = 0;
    table->items = (TableItem**)calloc((size_t)count, sizeof(TableItem*));
    for (int i = 0; i < count; i++) {
        table->items[i] = palloc(TableItem);
        table->items[i]->title = strdup(titles[i]);
        table->items[i]->row = hint_height + i + 1;
        table->items[i]->context = NULL;
        table->items[i]->callback = dummy;
    }
    return table;
}

PromptLine *getPL(const char *prompt, const char *line)
{
    PromptLine *pl = palloc(PromptLine);
    pl->prompt = prompt ? strdup(prompt) : NULL;
    pl->buffer = line ? strdup(line) : NULL;
    return pl;
}

void freePL(void *context)
{
    if (!context) return;
    PromptLine *pl = (PromptLine*)context;
    free(pl->prompt);
    free(pl->buffer);
    free(pl);
}

void freeTableItem(TableItem* item, void (*freeContext)(void *context))
{
    free(item->title);
    freeContext(item->context);
    free(item);
}

void freeTable(TableInterface *table, 
    void (*freeItemContext)(void *context),
    void (*freeTableContext)(void *context))
{
    for (int i = 0; i < table->count; i++) {
        freeTableItem(table->items[i], freeItemContext);
    }
    free(table->items);
    free(table->hint);
    freeTableContext(table->context);
    free(table);
}

void tableItemHighlight(TableItem *item)
{
    if (!item) return;
    highlight(item->title, item->row);
}

void tableItemDehighlight(TableItem *item)
{
    if (!item) return;
    dehighlight(item->title, item->row);
}

void editCallback(void *target)
{
    if (!target) return;
    TableItem *item = (TableItem*)target;
    if (!item->context) return;

    tableItemDehighlight(item);
    PromptLine *pl = (PromptLine*)item->context;
    char seq[64];
    snprintf(seq, 64, "\x1b[%d;1H\x1b[0K", item->row);
    write(STDOUT_FILENO, seq, strlen(seq));

    char *line = editLine(pl->prompt, pl->buffer);
    if (!line || line[0] == '\0') {
        free(line);
    } else {
        free(pl->buffer);
        pl->buffer = line;

        AppendBuffer ab = abInit();
        abAppend(&ab, pl->prompt, strlen(pl->prompt));
        abAppend(&ab, pl->buffer, strlen(pl->buffer));
        free(item->title);
        item->title = strndup(ab.buffer, ab.length);
        free(ab.buffer);
    }
    tableItemHighlight(item);
}

void passwordCallback(void *target)
{
    const static char *password_sign = "*************************";
    if (!target) return;
    TableItem *item = (TableItem*)target;
    if (!item->context) return;

    tableItemDehighlight(item);
    PromptLine *pl = (PromptLine*)item->context;
    char seq[64];
    snprintf(seq, 64, "\x1b[%d;1H\x1b[0K", item->row);
    write(STDOUT_FILENO, seq, strlen(seq));

    char *line = editPassword(pl->prompt);
    if (!line || line[0] == '\0') {
        free(line);
    } else {
        free(pl->buffer);
        pl->buffer = line;

        free(item->title);
        AppendBuffer ab = abInit();
        abAppend(&ab, pl->prompt, strlen(pl->prompt));
        abAppend(&ab, password_sign, strlen(pl->buffer));
        item->title = strndup(ab.buffer, ab.length);
        free(ab.buffer);
    }
    tableItemHighlight(item);
}

void drawTable(TableInterface *table)
{
    clearscr();
    printf("\x1b[1;1H%s\n", table->hint);
    int width = getwidth();
    int origin = table->display_origin;
    int bound = table->display_bound;
    char display[width + 1];
    for (int i = origin; i <= bound; i++) {
        char *title = table->items[i]->title;
        snprintf(display, width, "%s", title);
        if (i != table->focus) printf("%s", display);
        else printf("\x1b[47;30m%s\x1b[0m", display);
        if (i != bound) putchar('\n');
    }
    printf("\x1b[%d;%dH", getheight(), getwidth());
}

void tableUp(TableInterface *table)
{
    if (!table || table->focus <= 0) return;
    table->focus--;
    if (table->focus < table->display_origin) {
        table->display_origin--;
        table->display_bound--;
    }
    drawTable(table);
}

void tableDown(TableInterface *table)
{
    if (!table || table->focus >= table->count - 1) return;
    table->focus++;
    if (table->focus > table->display_bound) {
        table->display_bound++;
        table->display_origin++;
    }
    drawTable(table);
}

void tableDisplayAdjust(TableInterface *table)
{
    int height = getheight() - recalculateHintHeight(table->hint);
    int origin = table->display_origin;
    int bound = table->display_bound;
    bound = origin + height - 1;
    if (bound >= table->count) bound = table->count - 1;
    origin = bound - height + 1;
    if (origin < 0) origin = 0;
    int focus = table->focus;
    if (focus > bound) {
        origin += (focus - bound);
        bound = focus;
    } else if (focus < origin){
        bound -= (origin - focus);
        origin = focus;
    }
    table->display_origin = origin;
    table->display_bound = bound;
    drawTable(table);
}

DialogInterface *getDialog(const char *msg, DCallback agree, DCallback disagree)
{
    DialogInterface *dialog = palloc(DialogInterface);
    dialog->message = strdup(msg);
    dialog->agree = agree;
    dialog->disagree = disagree;
    return dialog;
}

void drawDialog(DialogInterface *dialog)
{
    clearscr();
    printf("\x1b[1;1H%s\n", dialog->message);
}

void freeDialog(DialogInterface *dialog)
{
    free(dialog->message);
    free(dialog);
}