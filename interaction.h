#ifndef _INTERACTION_H_
#define _INTERACTION_H_

typedef struct prompt_line_t {
	char *prompt;
	char *buffer;
} PromptLine;

typedef struct table_item_t {
	int row;
	char *title;
	void *context;
	void (*callback)(void *target);
} TableItem;

typedef struct table_interface_t {
	int count;
	int focus;
	int display_origin;
	int display_bound;
	void *context;
	char *hint;
	TableItem **items;
} TableInterface;

struct dialog_interface_t;
typedef void (*DCallback)(void *target);

typedef struct dialog_interface_t {
	void *context;
	char *message;
	DCallback agree;
	DCallback disagree;
} DialogInterface;

TableInterface *getMenuTable (int count, const char *titles[], const char *hint);
PromptLine *getPL(const char *prompt, const char *line);
void freePL(void *pl);
void freeTableItem(TableItem *item, void (*freeContext)(void *context));
void freeTable(TableInterface *table, DCallback freeItemContext, DCallback freeTableContext);

void drawTable(TableInterface *interface);
void tableUp(TableInterface *interface);
void tableDown(TableInterface *interface);
void tableDisplayAdjust(TableInterface *table);

void dummy(void *);
void editCallback(void *);
void passwordCallback(void *);

DialogInterface *getDialog(const char *msg, DCallback agree, DCallback disagree);
void drawDialog(DialogInterface *dialog);
void freeDialog(DialogInterface *dialog);

#endif /* _TABLE_INTERFACE_H_ */