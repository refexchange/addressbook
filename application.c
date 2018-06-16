//
// Created by refexchange on 2018/6/8.
//

#include "application.h"
#include "crypto.h"
#include "util.h"
#include "terminal.h"

UserList *userList = NULL;
User *currentUser = NULL;
AddressBook *addressBook = NULL;


#define USER_DEF_PATH "userdef.dat"

void saveUserList()
{
    size_t rlength;
    char *dumpstring = dumpUserList(userList, &rlength);
    rewriteFile(USER_DEF_PATH, dumpstring);
    free(dumpstring);
}

User *getCurrentUser()
{
    return currentUser;
}

User *loginUser(const char *username, const char *password)
{
    if (verifyUserCredentials(userList, username, password) == USER_OK) {
        currentUser = findUser(userList, username);
        char filename[64];
        snprintf(filename, 63, "%s.bin", currentUser->uid);
        if (access(filename, F_OK) == 0) {
            char *source = cryptoLoadFile(filename, password);
            addressBook = loadAddressBook(source, 0);
            free(source);
        } else {
            addressBook = loadAddressBook(NULL, 0);
        }
    } else {
        currentUser = NULL;
    }
    return currentUser;
}

User *registerUser(const char *username, const char *password)
{
    User *user = createUser(userList, username, password);
    verifyUserCredentials(userList, username, password);
    currentUser = user;
    addressBook = loadAddressBook(NULL, 0);
    saveUserList();
    return user;
}

AddressBook *getBook()
{
    return addressBook;
}

void saveBook()
{
    if (!currentUser) return;
    if (!addressBook) return;
    if (addressBook->contacts->count == 0) return;
    char *dumpstring = dumpAddressBook(addressBook);
    char filename[64];
    snprintf(filename, 64, "%s.bin", currentUser->uid);
    cryptoWriteFile(filename, dumpstring, currentUser->password);
    free(dumpstring);
}

void appInit()
{
    size_t filesize;
    if (access(USER_DEF_PATH, F_OK) == 0) {    
        char *user_file_content = getFileContent(USER_DEF_PATH, &filesize);
        userList = loadUserList(user_file_content);
    } else {
        userList = list_instance_new(UserList);
    }
}