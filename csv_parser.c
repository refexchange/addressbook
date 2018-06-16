//
// Created by refexchange on 2018/5/26.
//

#include "csv_parser.h"
#include <stdio.h>
#include <string.h>


char *csvDequote(char *source, size_t *rlength)
{
    /*
     * Stage 1: Scan source. If there are commas, set quote_flag to 1; If any quotation
     * marks are found, increase quomark_count.
     * */
    *rlength = 0;
    size_t length = strlen(source);
    int quoted_flag = 0;
    int quomark_count = 0;
    for (int i = 0; i < length; i++) {
        if (source[i] == ',') quoted_flag = 1;
        if (source[i] == '\"') quomark_count++;
    }
    if (quomark_count % 2 != 0) return NULL;
    /* Stage 2: If the token is quoted, then remove the quotation marks at 2 ends. */
    /* Attention: this process may alter source string. */
    if (quoted_flag) {
        if (source[0] != source[length - 1] && source[0] != '\"') return NULL;
        source[length - 1] = '\0';
        source += 1;
        quomark_count -= 2;
        length -= 2;
    }
    /* Stage 3: Parse. If quomarks that are not adjoining found, return an error. */
    char *ret = (char*)calloc(length - quomark_count / 2 + 1, sizeof(char));
    *rlength = length - quomark_count / 2;
    for (int src_index = 0, ret_index = 0; src_index < length; src_index++, ret_index++) {
        ret[ret_index] = source[src_index];
        if (source[src_index] == '\"') {
            if (source[src_index + 1] == '\"') {
                src_index++;
                continue;
            }
            free(ret);
            ret = NULL;
            *rlength = 0;
            break;
        }
    }
    if (quoted_flag) source[length] = '\"';
    return ret;
}

char *parseNextToken(char *source, size_t *rlength, int *next)
{
    int sep_index = 0;

    int i;
    for (i = 0; source[i] == '\"'; i++) {}
    if (i % 2 == 1) {
        int j = i;
        while (source[j] != '\0') {
            if (source[j] != '\"') {
                j++;
                continue;
            }
            int count;
            for (count = 0; source[j] == '\"'; j++, count++) {}
            if (count % 2 == 1) {
                sep_index = j;
                break;
            }
        }
    }

    for (; source[sep_index] != ',' && source[sep_index] != '\0'; sep_index++) {}
    char backup = source[sep_index];
    source[sep_index] = '\0';
    *next = sep_index + 1;
    char *token = csvDequote(source, rlength);
    source[sep_index] = backup;
    return token;
}

char **csvParseLine(char *source, size_t count, size_t *lengths)
{
    size_t length = strlen(source);
    int left_boundary = 0;
    int temp_boundary = 0;
    char **parse_result = (char**)calloc(count, sizeof(char*));
    for (int i = 0; i < count; i++) {
        if (left_boundary >= length) {
            parse_result[i] = NULL;
            continue;
        }
        parse_result[i] = parseNextToken(source + left_boundary, &lengths[i], &temp_boundary);
        left_boundary += temp_boundary;
    }
    return parse_result;
}

char *csvQuote(const char *source, size_t *rlength)
{
    size_t length = strlen(source);
    int comma_count = 0;
    int quote_count = 0;
    for (int i = 0; i < length; i++) {
        if (source[i] == ',') comma_count++;
        if (source[i] == '\"') quote_count++;
    }

    if (comma_count == 0 && quote_count == 0) {
        *rlength = length;
        return strdup(source);
    }

    if (comma_count > 0 && quote_count == 0) {
        *rlength = length + 2;
        char *ret = (char*)calloc(1 + (*rlength), sizeof(char));
        sprintf(ret, "\"%s\"", source);
        return ret;
    }

    if (comma_count == 0 && quote_count > 0) {
        *rlength = length + quote_count;
        char *ret = (char*)calloc(1 + (*rlength), sizeof(char));
        for (int i = 0, j = 0; i < length; i++, j++) {
            if (source[i] == '\"') {
                ret[j] = '\"';
                ret[++j] = '\"';
            } else {
                ret[j] = source[i];
            }
        }
        return ret;
    }

    if (comma_count > 0 && quote_count > 0) {
        *rlength = length + quote_count + 2;
        char *ret = (char*)calloc(1 + (*rlength), sizeof(char));
        ret[0] = '\"';
        int i, j;
        for (i = 0, j = 1; i < length; i++, j++) {
            if (source[i] == '\"') {
                ret[j] = '\"';
                ret[++j] = '\"';
            } else {
                ret[j] = source[i];
            }
        }
        ret[j] = '\"';
        return ret;
    }

    return NULL;
}

char *csvMakeLine(int count, const char **fields, size_t *rlength)
{
    char *serialized[count];
    size_t total_length = (size_t)count;
    size_t temp_length;
    for (int i = 0; i < count; i++) {
        serialized[i] = csvQuote(fields[i], &temp_length);
        total_length += temp_length;
    }
    *rlength = total_length;
    char *ret = (char*)calloc(total_length, sizeof(char));
    for (int i = 0; i < count - 1; i++) {
        strcat(ret, serialized[i]);
        strcat(ret, ",");
        free(serialized[i]);
    }
    strcat(ret, serialized[count - 1]);
    free(serialized[count - 1]);
    return ret;
}