//
// Created by refexchange on 2018/6/8.
//

#ifndef CONTACTS_APPLICATION_H
#define CONTACTS_APPLICATION_H

#include "user.h"
#include "addressbook.h"

User *getCurrentUser();
User *loginUser(const char *username, const char *password);
User *registerUser(const char *username, const char *password);

AddressBook *getBook();
void saveBook();

void appInit();

#endif //CONTACTS_APPLICATION_H
