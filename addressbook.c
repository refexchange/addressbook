//
// Created by refexchange on 2018/5/31.
//

#include "terminal.h"
#include "addressbook.h"

#pragma mark Contacts dump and load
#define PERSON_FIELD_VECT(x) { \
    (x)->firstName, \
    (x)->lastName, \
    (x)->fullName, \
    (x)->phone, \
    (x)->email, \
    (x)->address, \
    (x)->group \
}

Contact* initPerson(const char *firstName,
               const char *lastName, const char *fullName,
               const char *phone, const char *email,
               const char *address, const char *group)
{
    Contact *person = (Contact *) malloc(sizeof(Contact));
    const char *sourcebuffers[] = {
            firstName, lastName, fullName,
            phone, email,
            address, group
    };
    char *destbuffers[] = PERSON_FIELD_VECT(person);
    for (int i = 0; i < 7; i++) {
        if (sourcebuffers[i]) {
            strcpy(destbuffers[i], sourcebuffers[i]);
        } else {
            strcpy(destbuffers[i], "");
        }
    }

    if (!fullName || fullName[0] == '\0') {
        if (isEnglishName(firstName) && isEnglishName(lastName)) {
            sprintf(person->fullName, "%s %s", firstName, lastName);
        } else {
            sprintf(person->fullName, "%s%s", lastName, firstName);
        }
    } else {
        strcpy(person->fullName, fullName);
    }
    return person;
}

Contact *initPersonWithTokens(const char **t)
{
    return initPerson(t[0], t[1], t[2], t[3], t[4], t[5], t[6]);
}

char *dumpPerson(Contact *source, size_t *length)
{
    const char *serialize_source[] = PERSON_FIELD_VECT(source);
    return csvMakeLine(7, serialize_source, length);
}

char *dumpPeople(ContactList *source, size_t *length)
{
    char *serialized[source->count];
    size_t total_length = (size_t)source->count;
    size_t temp_length = 0;
    ContactEntry *entry = source->tail;
    for (int i = 0; i < source->count; i++) {
        serialized[i] = dumpPerson(entry->value, &temp_length);
        total_length += temp_length;
        entry = list_next_entry(entry);
    }
    *length = total_length;
    char *ret = (char*)calloc(total_length + 1, sizeof(char));
    for (int i = 0; i < source->count; i++) {
        strcat(ret, serialized[i]);
        if (i != source->count - 1) strcat(ret, "\n");
        free(serialized[i]);
    }
    return ret;
}

Contact *loadPerson(char *source)
{
    if (!source || source[0] == '\0' || strcmp(source, "\n") == 0) {
        return NULL;
    }
    size_t lengths[7];
    char **buffers = csvParseLine(source, 7, lengths);
    Contact *person = initPersonWithTokens((const char **)buffers);
    for (int i = 0; i < 7; i++) {
        free(buffers[i]);
    }
    free(buffers);
    return person;
}

ContactList *loadPeople(char *source)
{
    ContactList *list = list_instance_new(ContactList);
    while (*source != '\0') {
        int bound = 0;
        while (source[bound] != '\0' && source[bound] != '\n') {
            bound++;
        }
        char backup = source[bound];
        source[bound] = '\0';
        Contact *person = loadPerson(source);
        if (person) {
            ContactEntry *pentry = list_entry_new(ContactEntry, person);
            list_instance_append(list, pentry);
        }
        source[bound] = backup;
        if (backup == '\0') break;
        source = source + bound + 1;
    }
    return list;
}

unsigned int BKDRHash(const char *source)
{
    const static unsigned int seed = 131;
    unsigned int hash = 0;
    for (int i = 0; source[i] != '\0'; i++) {
        hash = hash * seed + source[i];
    }
    return hash % HASH_TABLE_SIZE;
}

#pragma mark Address Book

static ContactGroup *contact_group_init(const char *name)
{
    ContactGroup *group = (ContactGroup*)malloc(sizeof(ContactGroup));
    group->member = list_instance_new(ContactList);
    strcpy(group->name, name);
    return group;
}

static void book_init_table(AddressBook *book)
{
    if (!book) {
        return;
    }
    book->groupHashTable = (GroupList**)calloc(HASH_TABLE_SIZE, sizeof(GroupList*));
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        book->groupHashTable[i] = list_instance_new(GroupList);
    }
}

static int book_add_person_group(AddressBook *book, Contact *person)
{
    if (!book || !book->groupHashTable) {
        return 0;
    }
    ContactGroup *group = findGroup(book, person->group);
    ContactEntry *pentry = list_entry_new(ContactEntry, person);
    int hash = BKDRHash(person->group);
    if (!group) {
        group = contact_group_init(person->group);
        list_instance_append(group->member, pentry);
        GroupEntry *group_entry = list_entry_new(GroupEntry, group);
        GroupEntry *group_entry1 = list_entry_new(GroupEntry, group);
        if (!book->groupHashTable[hash]) {
            book->groupHashTable[hash] = list_instance_new(GroupList);
        }
        list_instance_append(book->groupHashTable[hash], group_entry);
        if (!book->groups) {
            book->groups = list_instance_new(GroupList);
        }
        list_instance_append(book->groups, group_entry1);
    } else {
        list_instance_append(group->member, pentry);
    }
    return hash;
}
static int matchAll(ContactEntry *entry, const char *keyword)
{
    Contact *person = entry->value;
    if (strstr(person->lastName, keyword)) {
        return 1;
    } else if (strstr(person->firstName, keyword)) {
        return 1;
    } else if (strstr(person->fullName, keyword)) {
        return 1;
    } else if (strstr(person->email, keyword)) {
        return 1;
    } else if (strstr(person->phone, keyword)) {
        return 1;
    } else if (strstr(person->address, keyword)) {
        return 1;
    } else if (strstr(person->group, keyword)) {
        return 1;
    }
    return 0;
}

int matchName(ContactEntry *entry, const char *keyword)
{
    return strstr(entry->value->fullName, keyword) ? 1 : 0;
}

int matchEmail(ContactEntry *entry, const char *keyword)
{
    return strstr(entry->value->email, keyword) ? 1 : 0;
}

int matchPhone(ContactEntry *entry, const char *keyword)
{
    return strstr(entry->value->phone, keyword) ? 1 : 0;
}

int matchAddress(ContactEntry *entry, const char *keyword)
{
    return strstr(entry->value->address, keyword) ? 1 : 0;
}

AddressBook *loadAddressBook(char *source, int ignore_firstline)
{
    AddressBook *book = (AddressBook*)malloc(sizeof(AddressBook));
    book->contacts = NULL;
    book->groups = NULL;
    book_init_table(book);
    if (!source) return book;

    if (ignore_firstline) {
        while (*source != '\n') source++;
        source++;
    }
    book->contacts = loadPeople(source);
    ContactEntry *entry = book->contacts->tail;
    for (int i = 0; i < book->contacts->count; i++) {
        Contact *person = entry->value;
        book_add_person_group(book, person);
        entry = list_next_entry(entry);
    }
    return book;
}

char *dumpAddressBook(AddressBook *book)
{
    if (book->contacts->count < 1) {
        return NULL;
    }
    size_t temp;
    return dumpPeople(book->contacts, &temp);
}

void addContact(AddressBook *book, Contact *person)
{
    if (!book) return;
    if (!book->groupHashTable) {
        book_init_table(book);
    }
    if (!book->contacts) {
        book->contacts = list_instance_new(ContactList);
    }
    ContactEntry *pentry = list_entry_new(ContactEntry, person);
    list_instance_append(book->contacts, pentry);
    book_add_person_group(book, person);
}

void removeContact(AddressBook *book, Contact *person)
{
    if (!book || !person) return;
    if (!book->contacts) return;
    ContactEntry *cur = book->contacts->tail;
    int found_flag = 0;
    for (int i = 0; i < book->contacts->count; i++) {
        if (cur->value == person) {
            found_flag = 1;
            break;
        }
        cur = list_next_entry(cur);
    }
    ContactList *list = book->contacts;
    if (found_flag) {
        if (cur == list->tail) {
            if (cur == list->head) list->tail = list->head = NULL;
            else list->tail = list_next_entry(cur);
        }
        if (cur == list->head) {
            list->head = list_prev_entry(cur);
        }
        list_unchain(cur);
        free(cur);
        book->contacts->count--;
    }
    ContactGroup *group = findGroup(book, person->group);
    found_flag = 0;
    if (group) {
        cur = group->member->tail;
        for (int i = 0; i < group->member->count; i++) {
            if (cur->value == person) {
                found_flag = 1;
                list = group->member;
                break;
            }
            cur = list_next_entry(cur);
        }
    }
    if (found_flag) {
        if (cur == list->tail) {
            if (cur == list->head) list->tail = list->head = NULL;
            else list->tail = list_next_entry(cur);
        }
        if (cur == list->head) {
            list->head = list_prev_entry(cur);
        }
        list_unchain(cur);
        group->member->count--;
        free(cur);
    }
    free(person);
}

ContactList *bookSearch(AddressBook *book, const char *keyword, Matcher match)
{
    ContactList *ret = list_instance_new(ContactList);
    ContactEntry *entry = book->contacts->tail;
    for (int i = 0; i < book->contacts->count; i++) {
        if (match(entry, keyword)) {
            ContactEntry *dup = list_entry_new(ContactEntry, entry->value);
            list_instance_append(ret, dup);
        }
        entry = list_next_entry(entry);
    }
    return ret;
}

void searchCleanup(ContactList *result)
{
    ContactEntry *entry = result->tail;
    ContactEntry *entries[result->count];
    for (int i = 0; i < result->count; i++) {
        entries[i] = entry;
        entry = list_next_entry(entry);
    }
    for (int i = 0; i < result->count; i++) {
        free(entries[i]);
    }
    free(result);
}

ContactGroup *findGroup(AddressBook *book, const char *group_name)
{
    if (!book->groupHashTable) {
        return NULL;
    }
    unsigned int hash = BKDRHash(group_name);
    GroupList *list = book->groupHashTable[hash];
    if (!list) {
        return NULL;
    }
    GroupEntry *entry = list->tail;
    for (int i = 0; i < list->count; i++) {
        if (strcmp(entry->value->name, group_name) == 0) {
            return entry->value;
        }
    }
    return NULL;
}