//
// Created by refexchange on 2018/6/3.
//

#include "user.h"
#include "util.h"
#include "csv_parser.h"

User *findUser(UserList *userList, const char *name)
{
    UserEntry *ret = userList->tail;
    int found_flag = 0;
    for (int i = 0; i < userList->count; i++) {
        if (strcmp(ret->value->username, name) == 0) {
            found_flag = 1;
            break;
        }
        ret = list_next_entry(ret);
    }
    return found_flag ? ret->value : NULL; 
}

User* createUser(UserList *userList, const char *name, const char *password)
{
    if (findUser(userList, name)) {
        return NULL;
    }
    
    User *user = palloc(User);
    user->password_hash = cryptoHash(password);
    user->uid = cryptoGenerateUUID();
    user->username = strdup(name);
    
    UserEntry *entry = list_entry_new(UserEntry, user);
    list_instance_append(userList, entry);
    return user;
}

int verifyUserCredentials(UserList *userList, const char *name, const char *password)
{
    User *user = findUser(userList, name);
    if (!user) {
        return USER_NOT_FOUND;
    }
    if (cryptoVerifyHash(password, user->password_hash) == YES) {
        user->password = strdup(password);
        return USER_OK;
    } else {
        return INCORRECT_PASSWORD;
    }
}

User *user_load(char *source)
{
    size_t lengths[3];
    char **parsed = csvParseLine(source, 3, lengths);
    if (!parsed) return NULL;
    User *user = palloc(User);
    user->username = strdup(parsed[0]);
    user->password = NULL;
    user->password_hash = strdup(parsed[1]);
    user->uid = strdup(parsed[2]);
    for (int i = 0; i < 3; i++) free(parsed[i]);
    free(parsed);
    return user;
}

UserList *loadUserList(char *source)
{
    UserList *list = list_instance_new(UserList);
    while (*source != '\0') {
        int bound = 0;
        while (source[bound] != '\0' && source[bound] != '\n') {
            bound++;
        }
        char backup = source[bound];
        source[bound] = '\0';
        User *user = user_load(source);
        if (user) {
            UserEntry *entry = list_entry_new(UserEntry, user);
            list_instance_append(list, entry);
        }
        source[bound] = backup;
        if (backup == '\0') break;
        source = source + bound + 1;
    }
    return list;
}

char *user_dump(User *source, size_t *rlength)
{
    char *buffers[3] = {
            source->username,
            source->password_hash,
            source->uid,
    };
    return csvMakeLine(3, (const char **) buffers, rlength);
}

char *dumpUserList(UserList *source, size_t *rlength)
{
    char *lines[source->count];
    size_t lengths[source->count];
    size_t total_length = (size_t)source->count;
    UserEntry *entry = source->tail;
    for (int i = 0; i < source->count; i++) {
        lines[i] = user_dump(entry->value, &lengths[i]);
        total_length += lengths[i];
        entry = list_next_entry(entry);
    }
    char *ret = stralloc(total_length);
    for (int i = 0; i < source->count; i++) {
        strcat(ret, lines[i]);
        if (i < source->count - 1) strcat(ret, "\n");
    }
    utilFreeStrv(source->count, lines);
    if (rlength) *rlength = total_length;
    return ret;
}