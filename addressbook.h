//
// Created by refexchange on 2018/5/31.
//

#ifndef CONTACTS_ADDRESSBOOK_H
#define CONTACTS_ADDRESSBOOK_H

#include "list.h"
#include "csv_parser.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_USAGE
typedef struct person_t {
    char firstName[32];
    char lastName[32];
    char fullName[64];
    char phone[32];
    char email[64];
    char address[128];
    char group[128];
} Contact;

typedef struct person_entry_t {
    list_node node;
    Contact *value;
} ContactEntry;

typedef struct person_list_t {
    ContactEntry *head;
    ContactEntry *tail;
    int count;
} ContactList;

typedef struct contact_group_t {
    char name[128];
    ContactList *member;
} ContactGroup;

typedef struct group_entry_t {
    list_node node;
    ContactGroup *value;
} GroupEntry;

typedef struct group_list_t {
    GroupEntry *head;
    GroupEntry *tail;
    int count;
} GroupList;

#define HASH_TABLE_SIZE 233

typedef struct addressbook_t {
    ContactList *contacts;
    GroupList *groups;
    GroupList **groupHashTable;
} AddressBook;

Contact* initPerson(const char *firstName,
                   const char *lastName, const char *fullName,
                   const char *phone, const char *email,
                   const char *address, const char *group);

AddressBook *loadAddressBook(char *source, int ignore_firstline);
char *dumpAddressBook(AddressBook *book);
ContactList *loadPeople(char *source);

void addContact(AddressBook *book, Contact *person);
void removeContact(AddressBook *book, Contact *person);

typedef int (*Matcher)(ContactEntry *, const char *);
int matchName(ContactEntry *entry, const char *keyword);
int matchPhone(ContactEntry *entry, const char *keyword);
int matchEmail(ContactEntry *entry, const char *keyword);
int matchAddress(ContactEntry *entry, const char *keyword);

ContactList *bookSearch(AddressBook *book, const char *keyword, Matcher match);
void searchCleanup(ContactList *result);
ContactGroup *findGroup(AddressBook *book, const char *group_name);

#endif //CONTACTS_ADDRESSBOOK_H
