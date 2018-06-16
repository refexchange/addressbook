//
// Created by refexchange on 2018/5/9.
//

#ifndef CONTACTS_UTIL_H
#define CONTACTS_UTIL_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define palloc(type) (type*)calloc(1, sizeof(type))
#define stralloc(count) (char*)calloc((count), sizeof(char))
#define dupifset(x) ((x) ? strdup(x) : strdup(""))
#define isalpha(x) (((x) >= 'a' && (x) <= 'z') || ((x) >= 'A' && (x) <= 'Z') || (x) == '\'')

#define YES 1
#define NO 0


void i32toa(int32_t, char*);
void i64toa(int64_t, char*);
void u32toa(uint32_t, char*);
void u64toa(uint64_t, char*);

int isEnglishName(const char *name);
void utilFreeStrv(int count, char **source);

size_t getFileSize(const char *filename);
size_t rewriteFile(const char *filename, const char *content);
char* getFileContent(const char *filename, size_t *length);

#endif //CONTACTS_UTIL_H
