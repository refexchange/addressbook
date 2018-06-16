//
// Created by refexchange on 2018/5/8.
//

#ifndef CONTACTS_USER_H
#define CONTACTS_USER_H

#include "list.h"
#include "crypto.h"

typedef struct user_t
{
    char *username;
    char *password;
    char *password_hash;
    char *uid;
} User;

typedef struct user_list_entry_t {
    list_node node;
    User *value;
} UserEntry;

typedef struct user_list_t {
    UserEntry *head;
    UserEntry *tail;
    int count;
} UserList;

#define USER_OK 0
#define USER_NOT_FOUND 1
#define INCORRECT_PASSWORD 2

User *findUser(UserList *userList, const char *name);
User* createUser(UserList *userList, const char *name, const char *password);
int verifyUserCredentials(UserList *userList, const char *name, const char *password);
UserList *loadUserList(char *source);
char *dumpUserList(UserList *source, size_t *rlength);

#endif //CONTACT_USER_H
