//
// Created by refexchange on 2018/5/26.
//

#ifndef CONTACTS_CSV_PARSER_H
#define CONTACTS_CSV_PARSER_H

#include <stdlib.h>

char *csvQuote(const char *source, size_t *rlength);
char *csvMakeLine(int count, const char **fields, size_t *rlength);
char *csvDequote(char *source, size_t *rlength);
char **csvParseLine(char *source, size_t count, size_t *lengths);

#endif //CONTACTS_CSV_PARSER_H
