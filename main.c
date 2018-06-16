#include "interaction.h"
#include "termanip.h"
#include "editline.h"
#include "application.h"
#include "addressbook.h"
#include "user.h"
#include "list.h"
#include "util.h"

#define IS_EMPTY_STRING(x) (!(x) || (x)[0] == '\0')

TableInterface *curTable = NULL;
DialogInterface *curDialog = NULL;
int cleanup_flag = 0;

TableInterface *loginMenu = NULL;
TableInterface *loginPage = NULL;
TableInterface *registerPage = NULL;
TableInterface *mainMenuPage = NULL;

#define TABLE_MODE 1
#define DIALOG_MODE 2
#define UNKNOWN_MODE 3

int curMode = UNKNOWN_MODE;

void setTable(TableInterface *table)
{
    curMode = TABLE_MODE;
    curTable = table;
}

void setDialog(DialogInterface *dialog)
{
    curMode = DIALOG_MODE;
    curDialog = dialog;
}

#pragma mark Pre-Defination of functions.

#define GOTO_MAIN_MENU 1
#define GOTO_LOGIN_MENU 2
DialogInterface *allocAlert(const char *message, int how);
TableInterface *allocAddContactPage();
TableInterface *allocContactPage(Contact *person);
TableInterface *allocSearchPage();
TableInterface *allocContactTable(ContactList *people);
TableInterface *allocGroupTable(GroupList *groups);
TableInterface *allocStatisticPage();
TableInterface *allocAboutMePage();
TableInterface *allocImportPage();
TableInterface *allocExportPage();

void mainMenuSelectCallback(void *caller);
void addContactCallback(void *caller);
void saveContactCallback(void *caller);
void selectContactCallback(void *caller);
void selectGroupCallback(void *caller);
void searchCallback(void *caller);
void cancelAndGotoMainMenu(void *caller);
void importCallback(void *caller);
void exportCallback(void *caller);

void backToLoginMenu(void *dontcare) { setTable(loginMenu); }
void backToMainMenu(void *dontcare) { setTable(mainMenuPage); }
void gotoLoginPage(void *dontcare) { setTable(loginPage); }
void gotoRegisterPage(void *dontcare) { setTable(registerPage); }

#pragma mark Interface Callbacks Definition

void clearAlert1(void *dontcare);
void clearAlert2(void *dontcare);

void mainMenuSelectCallback(void *caller)
{
    int focus = curTable->focus;
    if (focus == 0) { /* list all */
        setTable(allocContactTable(getBook()->contacts));
    } else if (focus == 1) { /* list group */
        setTable(allocGroupTable(getBook()->groups));
    } else if (focus == 2) { /* add contact */
        setTable(allocAddContactPage());
    } else if (focus == 3) { /* search contact */
        setTable(allocSearchPage());
    } else if (focus == 4) { /* export */
        setTable(allocExportPage());
    } else if (focus == 5) { /* import */
        setTable(allocImportPage());
    } else if (focus == 6) { /* statistic */
        setTable(allocStatisticPage());
    } else if (focus == 7) { /* about me */
        setTable(allocAboutMePage());
    }
}

void searchCleanupAdapter(void *caller)
{
    ContactList *people = (ContactList*)curTable->context;
    searchCleanup(people);
    backToMainMenu(caller);
    cleanup_flag = 0;
}

void selectContactCallback(void *caller)
{
    TableItem *item = (TableItem*)caller;
    Contact *person = (Contact*)item->context;
    freeTable(curTable, dummy, dummy);
    setTable(allocContactPage(person));
}

void importCallback(void *caller)
{
    AddressBook *book = getBook();
    TableItem *item = curTable->items[0];
    PromptLine *filename_pl = (PromptLine*)item->context;
    char *filename = filename_pl->buffer;
    size_t size;
    char *content = getFileContent(filename, &size);
    char *backup = content;
    while (*(content - 1) != '\n' && *content != '\0') content++;
    ContactList *people = loadPeople(content);
    free(backup);

    ContactEntry *entry = people->tail;
    for (int i = 0; i < people->count; i++) {
        addContact(book, entry->value);
        entry = list_next_entry(entry);
    }
    saveBook();
    searchCleanup(people);
    cancelAndGotoMainMenu(caller);
}

void exportCallback(void *caller)
{
    const static char *title_line =
            "First Name,Last Name,Full Name,Phone,Email,Address,Group\n";
    AddressBook *book = getBook();
    if (!book || book->contacts->count <= 0) {
        cancelAndGotoMainMenu(caller);
        return;
    }
    TableItem *item = curTable->items[0];
    PromptLine *filename_pl = (PromptLine*)item->context;
    char *filename = filename_pl->buffer;
    char *dumpstring = dumpAddressBook(book);
    size_t size = strlen(title_line) + strlen(dumpstring);
    char *content = stralloc(size + 1);
    sprintf(content, "%s%s", title_line, dumpstring);
    rewriteFile(filename, content);
    free(dumpstring);
    free(content);
    cancelAndGotoMainMenu(caller);
}

void selectGroupCallback(void *caller)
{
    TableItem *item = (TableItem*)caller;
    ContactGroup *group = (ContactGroup*)item->context;
    freeTable(curTable, dummy, dummy);
    setTable(allocContactTable(group->member));
}

void searchCallback(void *caller)
{
    char *buffers[4];
    for (int i = 0; i < 4; i++) {
        PromptLine *pl = (PromptLine*)curTable->items[i]->context;
        buffers[i] = pl->buffer;
    }
    ContactList *result = NULL;
    if (buffers[0]) {
        result = bookSearch(getBook(), buffers[0], matchName);
    } else if (buffers[1]) {
        result = bookSearch(getBook(), buffers[1], matchPhone);
    } else if (buffers[2]) {
        result = bookSearch(getBook(), buffers[2], matchEmail);
    } else if (buffers[3]) {
        result = bookSearch(getBook(), buffers[3], matchAddress);
    } else {
        setDialog(allocAlert("请填写搜索关键字！", GOTO_MAIN_MENU));
        freeTable(curTable, dummy, dummy);
        curTable = NULL;
        return;
    }
    cleanup_flag = 1;
    setTable(allocContactTable(result));
}

void saveContactCallback(void *caller)
{
    Contact *person = (Contact*)curTable->context;
    removeContact(getBook(), person);

    char *buffers[7];
    for (int i = 0; i < 7; i++) {
        PromptLine *pl = (PromptLine*)curTable->items[i]->context;
        buffers[i] = pl->buffer;
    }

    person = initPerson(buffers[0], buffers[1], buffers[2], buffers[3],
        buffers[4], buffers[5], buffers[6]);
    addContact(getBook(), person);
    saveBook();
    cancelAndGotoMainMenu(caller);
}

void deleteContact(void *caller)
{
    Contact *person = (Contact*)curDialog->context;
    if (person) {
        removeContact(getBook(), person);
        saveBook();
        freeDialog(curDialog);
        cancelAndGotoMainMenu(caller);
    }
}

void deleteContactCallback(void *caller)
{
    Contact *person = (Contact*)curTable->context;
    static char warning[256];
    snprintf(warning, 256, "即将删除联系人\"%s\"。\n按下回车确认，按下退格键放弃。", person->fullName);
    DialogInterface *dialog = getDialog(warning, deleteContact, clearAlert1);
    dialog->context = person;
    setDialog(dialog);
}

void cancelAndGotoMainMenu(void *caller)
{
    freeTable(curTable, dummy, dummy);
    setTable(mainMenuPage);
}

void addContactCallback(void *caller)
{
    char *fields[7];
    for (int i = 0; i < 7; i++) {
        PromptLine *pl = (PromptLine*)curTable->items[i]->context;
        fields[i] = pl->buffer;
    }
    Contact *person = initPerson(fields[0], fields[1], fields[2], fields[3], 
        fields[4], fields[5], fields[6]);
    addContact(getBook(), person);
    cancelAndGotoMainMenu(caller);
    saveBook();
}

void loginCallback(void *sender)
{
    TableItem *username_field = curTable->items[0];
    TableItem *password_field = curTable->items[1];
    PromptLine *username_pl = (PromptLine*)username_field->context;
    PromptLine *password_pl = (PromptLine*)password_field->context;

    if (IS_EMPTY_STRING(username_pl->buffer)) {
        setDialog(allocAlert("用户名不能为空！", GOTO_LOGIN_MENU));
        goto cleanup;
    }

    if (IS_EMPTY_STRING(password_pl->buffer)) {
        setDialog(allocAlert("密码不能为空！", GOTO_LOGIN_MENU));
        goto cleanup;
    }

    char *username = strdup(username_pl->buffer);
    char *password = strdup(password_pl->buffer);


    User *user = loginUser(username, password);
    free(username);
    free(password);
    if (!user) {
        setDialog(allocAlert("用户名或密码错误！请重新登录。", GOTO_LOGIN_MENU));
    } else {
        backToMainMenu(NULL);
    }
    goto cleanup;

cleanup:
    free(username_pl->buffer);
    free(password_pl->buffer);
    free(username_field->title);
    free(password_field->title);
    username_pl->buffer = NULL;
    password_pl->buffer = NULL;
    username_field->title = strdup(username_pl->prompt);
    password_field->title = strdup(password_pl->prompt);
}

void registerCallback(void *view)
{
    TableItem *username_field = curTable->items[0];
    TableItem *password_field = curTable->items[1];
    TableItem *confirm_field = curTable->items[2];
    PromptLine *username_pl = (PromptLine*)username_field->context;
    PromptLine *password_pl = (PromptLine*)password_field->context;
    PromptLine *confirm_pl = (PromptLine*)confirm_field->context;

    if (IS_EMPTY_STRING(username_pl->buffer)) {
        setDialog(allocAlert("用户名不能为空！", GOTO_LOGIN_MENU));
        goto cleanup;
    }

    if (IS_EMPTY_STRING(password_pl->buffer)) {
        setDialog(allocAlert("密码不能为空！", GOTO_LOGIN_MENU));
        goto cleanup;
    }

    if (IS_EMPTY_STRING(confirm_pl->buffer)) {
        setDialog(allocAlert("重复密码不能为空!", GOTO_LOGIN_MENU));
        goto cleanup;
    }

    char *username = strdup(username_pl->buffer);
    char *password = strdup(password_pl->buffer);
    char *confirm = strdup(confirm_pl->buffer);

    User *user = NULL;
    if (strcmp(password, confirm) != 0) {
        setDialog(allocAlert("密码和确认密码不一致！", GOTO_LOGIN_MENU));
    } else {
        user = registerUser(username, password);
    }

    free(username);
    free(password);
    free(confirm);

    if (!user) {
        setDialog(allocAlert("用户名已经存在，注册失败！", GOTO_LOGIN_MENU));
    } else {
        backToMainMenu(NULL);
    }
    goto cleanup;

cleanup:
    free(username_pl->buffer);
    free(password_pl->buffer);
    free(confirm_pl->buffer);
    free(username_field->title);
    free(password_field->title);
    free(confirm_field->title);
    username_pl->buffer = NULL;
    password_pl->buffer = NULL;
    confirm_pl->buffer = NULL;
    username_field->title = strdup(username_pl->prompt);
    password_field->title = strdup(password_pl->prompt);
    confirm_field->title = strdup(confirm_pl->prompt);
}

void exitCallback(void *dontcare)
{
    curMode = UNKNOWN_MODE;
}

#pragma mark Interface Allocator Definition

TableInterface *allocImportPage()
{
    const static char *hint = "输入你要导入的文件名。\n导入的文件必须是有7列的CSV格式文件，且文件必须和程序在同一个目录下。";
    const static char *prompts[] = {
        "文件名：",
        "导入",
        "返回上一级菜单..."
    };
    TableInterface *table = getMenuTable(3, prompts, hint);
    table->items[0]->context = getPL(prompts[0], NULL);
    table->items[0]->callback = editCallback;
    table->items[1]->callback = importCallback;
    table->items[2]->callback = cancelAndGotoMainMenu;
    return table;
}

TableInterface *allocExportPage()
{
    const static char *hint = "输入你要导出的文件名。\n导出的文件将以CSV格式存储于在程序所在目录下。";
    const static char *prompts[] = {
        "文件名：",
        "导出",
        "返回上一级菜单..."
    };
    TableInterface *table = getMenuTable(3, prompts, hint);
    table->items[0]->context = getPL(prompts[0], NULL);
    table->items[0]->callback = editCallback;
    table->items[1]->callback = exportCallback;
    table->items[2]->callback = cancelAndGotoMainMenu;
    return table;
}

TableInterface *allocAboutMePage()
{
    const static char *hint = "关于我";
    const static char *titles[] = {
        "GitHub: github.com/refexchange",
        "Email: graph_lin@outlook.com",
        "返回上一级菜单..."
    };
    TableInterface *table = getMenuTable(3, (const char**)titles, hint);
    table->items[2]->callback = cancelAndGotoMainMenu;
    return table;
}

TableInterface *allocAddContactPage()
{
    const static char *hint = "输入联系人信息。";
    const static char *prompts[] = {
        "          名字：",
        "          姓氏：",
        "全名（可留空）：",
        "          电话：",
        "      电子邮件：",
        "          地址：",
        "          分组：",
        "    新建联系人",
        "    放弃"
    };
    TableInterface *table = getMenuTable(9, prompts, hint);
    for (int i = 0; i < 7; i++) {
        table->items[i]->callback = editCallback;
        table->items[i]->context = getPL(prompts[i], NULL);
    }
    table->items[7]->callback = addContactCallback;
    table->items[8]->callback = cancelAndGotoMainMenu;
    return table;
}

TableInterface *allocContactTable(ContactList *people)
{
    if (!people || people->count <= 0) {
        const static char *hint = "你还没有添加任何联系人！";
        const static char *titles[1] = {
            "返回上一级菜单..."
        };
        TableInterface *table = getMenuTable(1, titles, hint);
        table->items[0]->callback = cancelAndGotoMainMenu;
        return table;
    }

    const static char *hint = "用方向键选择联系人，或者返回上一级菜单。";
    char **titles = (char**)calloc(people->count + 1, sizeof(char*));
    titles[0] = strdup("返回上一级菜单...");
    ContactEntry *entry = people->tail;
    for (int i = 1; i <= people->count; i++) {
        titles[i] = strdup(entry->value->fullName);
        entry = list_next_entry(entry);
    }
    TableInterface *table = getMenuTable(people->count + 1, (const char **)titles, hint);
    table->context = people;

    if (cleanup_flag) {
        table->items[0]->callback = searchCleanupAdapter;
    } else {
        table->items[0]->callback = cancelAndGotoMainMenu;
    }
    entry = people->tail;
    for (int i = 1; i <= people->count; i++) {
        table->items[i]->callback = selectContactCallback;
        table->items[i]->context = entry->value;
        entry = list_next_entry(entry);
    }
    for (int i = 0; i < people->count; i++) {
        free(titles[i]);
    }
    free(titles);
    return table;
}

TableInterface *allocGroupTable(GroupList *groups)
{
    if (!groups || groups->count <= 0) {
        const static char *hint = "你还没有添加任何联系人！";
        const static char *titles[1] = {
            "返回上一级菜单..."
        };
        TableInterface *table = getMenuTable(1, titles, hint);
        table->items[0]->callback = cancelAndGotoMainMenu;
        return table;
    }

    const static char *hint = "用方向键选择分组，或者返回上一级菜单。";
    char **titles = (char**)calloc(groups->count + 1, sizeof(char*));
    titles[0] = strdup("返回上一级菜单...");
    GroupEntry *entry = groups->tail;
    for (int i = 1; i <= groups->count; i++) {
        titles[i] = strdup(entry->value->name);
        entry = list_next_entry(entry);
    }
    TableInterface *table = getMenuTable(groups->count + 1, (const char**)titles, hint);
    table->items[0]->callback = cancelAndGotoMainMenu;
    entry = groups->tail;
    for (int i = 1; i <= groups->count; i++) {
        table->items[i]->callback = selectGroupCallback;
        table->items[i]->context = entry->value;
        entry = list_next_entry(entry);
    }
    for (int i = 0; i < groups->count; i++) {
        free(titles[i]);
    }
    free(titles);
    return table;
}

TableInterface *allocSearchPage()
{
    const static char *hint = "注意：联系人只支持单个字段的搜索。";
    const static char *options[] = {
        "    按姓名搜索：",
        "按电话号码搜索：",
        "    按电邮搜索：",
        "    按地址搜索：",
        "    搜索",
        "    放弃"
    };
    TableInterface *table = getMenuTable(6, options, hint);
    for (int i = 0; i < 4; i++) {
        table->items[i]->callback = editCallback;
        table->items[i]->context = getPL(options[i], NULL);
    }
    table->items[4]->callback = searchCallback;
    table->items[5]->callback = cancelAndGotoMainMenu;
    return table;
}

TableInterface *allocContactPage(Contact *person)
{
    char *buffers[7] = {
        person->firstName,
        person->lastName,
        person->fullName,
        person->phone,
        person->email,
        person->address,
        person->group
    };
    const static char *hint = "用方向键和回车选择你要更改的字段。";
    const static char *prompts[] = {
        "    名字：",
        "    姓氏：",
        "    全名：",
        "    电话：",
        "电子邮件：",
        "    地址：",
        "    分组："
    };
    char *titles[10];
    for (int i = 0; i < 7; i++) {
        titles[i] = malloc(256);
        snprintf(titles[i], 256, "%s%s", prompts[i], buffers[i]);
    }
    titles[7] = strdup("    删除联系人");
    titles[8] = strdup("    保存更改");
    titles[9] = strdup("    放弃更改");
    TableInterface *table = getMenuTable(10, (const char**)titles, hint);
    for (int i = 0; i < 7; i++) {
        table->items[i]->callback = editCallback;
        table->items[i]->context = getPL(prompts[i], buffers[i]);
    }
    table->items[7]->callback = deleteContactCallback;
    table->items[8]->callback = saveContactCallback;
    table->items[9]->callback = cancelAndGotoMainMenu;
    table->context = person;
    return table;
}

TableInterface *allocLoginMenu()
{
    const char *hint = "在使用之前你必须先登录。";
    const char *options[] = {
        "登录",
        "注册",
        "退出"
    };
    TableInterface *table = getMenuTable(3, options, hint);
    table->items[0]->callback = gotoLoginPage;
    table->items[1]->callback = gotoRegisterPage;
    table->items[2]->callback = exitCallback;
    return table;
}

TableInterface *allocLoginPage()
{
    const char *hint = "请输入你的用户名和密码。";
    const char *options[] = {
        "用户名：",
        "  密码：",
        "  登录",
        "  返回",
    };
    TableInterface *table = getMenuTable(4, options, hint);
    table->items[0]->context = getPL(options[0], NULL);
    table->items[1]->context = getPL(options[1], NULL);
    table->items[0]->callback = editCallback;
    table->items[1]->callback = passwordCallback;
    table->items[2]->callback = loginCallback;
    table->items[3]->callback = backToLoginMenu;
    return table;
}

TableInterface *allocRegisterPage()
{
    const char *hint = "请输入新的用户名和密码以注册。";
    const char *options[] = {
        "  用户名：",
        "    密码：",
        "重复密码：",
        "    注册",
        "    返回",
    };
    TableInterface *table = getMenuTable(5, options, hint);
    table->items[0]->context = getPL(options[0], NULL);
    table->items[1]->context = getPL(options[1], NULL);
    table->items[2]->context = getPL(options[2], NULL);
    table->items[0]->callback = editCallback;
    table->items[1]->callback = passwordCallback;
    table->items[2]->callback = passwordCallback;
    table->items[3]->callback = registerCallback;
    table->items[4]->callback = backToLoginMenu;
    return table;
}

TableInterface *allocMainMenuPage()
{
    const static char *hint = "请使用方向键选择所需的操作。";
    const static char *options[] = {
        "列出所有联系人",
        "列出所有联系人组",
        "添加联系人",
        "搜索联系人",
        "导出联系人",
        "导入联系人",
        "统计数据",
        "关于作者",
        "退出",
    };
    TableInterface *table = getMenuTable(9, options, hint);
    for (int i = 0; i < 8; i++) table->items[i]->callback = mainMenuSelectCallback;
    table->items[8]->callback = exitCallback;
    return table;
}

TableInterface *allocStatisticPage()
{
    const static char *hint = "下面展示各个分组的统计数据：";
    AddressBook *book = getBook();
    if (!book || book->contacts->count <= 0) {
        const static char *titles[2] = { "总计 : 0个联系人", "返回上一级菜单..." };
        TableInterface *table = getMenuTable(2, titles, hint);
        table->items[1]->callback = cancelAndGotoMainMenu;
        return table;
    }
    size_t count = book->groups->count;
    char *titles[count + 2];
    GroupEntry *entry = book->groups->tail;
    size_t total = 0;
    for (int i = 0; i < count; i++) {
        titles[i] = malloc(256);
        char *name = entry->value->name;
        size_t member_count = entry->value->member->count;
        snprintf(titles[i], 256, "%s : %zu个联系人", name, member_count);
        entry = list_next_entry(entry);
        total += member_count;
    }
    titles[count] = malloc(48);
    snprintf(titles[count], 48, "总计 : %zu个联系人", total);
    titles[count + 1] = strdup("返回上一级菜单...");

    TableInterface *table = getMenuTable(count + 2, (const char **)titles, hint);
    table->items[count + 1]->callback = cancelAndGotoMainMenu;
    for (int i = 0; i < count + 2; i++) {
        free(titles[i]);
    }
    return table;
}

void clearAlert1(void *dontcare)
{
    freeDialog(curDialog);
    curDialog = NULL;
    backToMainMenu(NULL);
}

void clearAlert2(void *dontcare)
{
    freeDialog(curDialog);
    curDialog = NULL;
    backToLoginMenu(NULL);
}

DialogInterface *allocAlert(const char *message, int how)
{
    static char *hint1 = "\n按下回车键返回主菜单。";
    static char *hint2 = "\n按下回车键返回登录菜单。";
    DialogInterface *alert;
    if (how == GOTO_MAIN_MENU) {
        char *composed = (char*)calloc(strlen(message) + strlen(hint1) + 1, sizeof(char));
        sprintf(composed, "%s%s", message, hint1);
        alert = getDialog(composed, clearAlert1, clearAlert1);
        free(composed);
    } else {
        char *composed = (char*)calloc(strlen(message) + strlen(hint2) + 1, sizeof(char));
        sprintf(composed, "%s%s", message, hint2);
        alert = getDialog(composed, clearAlert2, clearAlert2);
        free(composed);
    }
    return alert;
}

#pragma mark Interface Manipulating

void cliInit()
{
    loginMenu = allocLoginMenu();
    loginPage = allocLoginPage();
    registerPage = allocRegisterPage();
    mainMenuPage = allocMainMenuPage();
    curMode = TABLE_MODE;
    curTable = loginMenu;
}

void runLoop()
{
    cbreakmode();
    while (1) {
        if (curMode == TABLE_MODE) {
            drawTable(curTable);
            while (1) {
                int key = keyboardPress();
                if (key == KeyActionUp) {
                    tableUp(curTable);
                } else if (key == KeyActionDown) {
                    tableDown(curTable);
                } else if (key == KeyActionEnter) {
                    TableItem *item = curTable->items[curTable->focus];
                    item->callback(item);
                    break;
                }
            }
        } else if (curMode == DIALOG_MODE) {
            drawDialog(curDialog);
            while (1) {
                int keycode = keyboardPress();
                if (keycode == KeyActionBackspace) {
                    curDialog->disagree(curDialog);
                    break;
                } else if (keycode == KeyActionEnter) {
                    curDialog->agree(curDialog);
                    break;
                }
            }
        } else break;
    }
    resetmode();
}

void adjustTable(int signo)
{
    updatewindowsize();
    if (curMode != TABLE_MODE) return;
    tableDisplayAdjust(curTable);
}

int main(int argc, const char *argv[])
{
    signal(SIGWINCH, adjustTable);
    appInit();
    cliInit();
    runLoop();
    clearscr();
    printf("\x1b[1;1H");
    return 0;
}