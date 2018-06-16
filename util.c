// Super fast integer to string converts. Up to 23x faster than sprintf.


#include <stdlib.h>
#include <stdio.h>
#include "util.h"

const char gDigitsLut[200] = {
        '0','0','0','1','0','2','0','3','0','4','0','5','0','6','0','7','0','8','0','9',
        '1','0','1','1','1','2','1','3','1','4','1','5','1','6','1','7','1','8','1','9',
        '2','0','2','1','2','2','2','3','2','4','2','5','2','6','2','7','2','8','2','9',
        '3','0','3','1','3','2','3','3','3','4','3','5','3','6','3','7','3','8','3','9',
        '4','0','4','1','4','2','4','3','4','4','4','5','4','6','4','7','4','8','4','9',
        '5','0','5','1','5','2','5','3','5','4','5','5','5','6','5','7','5','8','5','9',
        '6','0','6','1','6','2','6','3','6','4','6','5','6','6','6','7','6','8','6','9',
        '7','0','7','1','7','2','7','3','7','4','7','5','7','6','7','7','7','8','7','9',
        '8','0','8','1','8','2','8','3','8','4','8','5','8','6','8','7','8','8','8','9',
        '9','0','9','1','9','2','9','3','9','4','9','5','9','6','9','7','9','8','9','9'
};

#define BEGIN2(n) \
    do { \
        int t = (n); \
        if(t < 10) *p++ = '0' + t; \
        else { \
            t *= 2; \
            *p++ = gDigitsLut[t]; \
            *p++ = gDigitsLut[t + 1]; \
        } \
    } while(0)

#define MIDDLE2(n) \
    do { \
        int t = (n) * 2; \
        *p++ = gDigitsLut[t]; \
        *p++ = gDigitsLut[t + 1]; \
    } while(0)

#define BEGIN4(n) \
    do { \
        int t4 = (n); \
        if(t4 < 100) BEGIN2(t4); \
        else { BEGIN2(t4 / 100); MIDDLE2(t4 % 100); } \
    } while(0)

#define MIDDLE4(n) \
    do { \
        int t4 = (n); \
        MIDDLE2(t4 / 100); MIDDLE2(t4 % 100); \
    } while(0)

#define BEGIN8(n) \
    do { \
        uint32_t t8 = (n); \
        if(t8 < 10000) BEGIN4(t8); \
        else { BEGIN4(t8 / 10000); MIDDLE4(t8 % 10000); } \
    } while(0)

#define MIDDLE8(n) \
    do { \
        uint32_t t8 = (n); \
        MIDDLE4(t8 / 10000); MIDDLE4(t8 % 10000); \
    } while(0)

#define MIDDLE16(n) \
    do { \
        uint64_t t16 = (n); \
        MIDDLE8(t16 / 100000000); MIDDLE8(t16 % 100000000); \
    } while(0)

void u32toa(uint32_t x, char* p)
{
    if(x < 100000000) BEGIN8(x);
    else { BEGIN2(x / 100000000); MIDDLE8(x % 100000000); }
    *p = 0;
}

void i32toa(int32_t x, char* p)
{
    uint64_t t;
    if(x >= 0) t = x;
    else *p++ = '-', t = -(uint32_t)(x);
    u32toa(t, p);

}

void u64toa(uint64_t x, char* p)
{
    if(x < 100000000) BEGIN8(x);
    else if(x < 10000000000000000) { BEGIN8(x / 100000000); MIDDLE8(x % 100000000); }
    else { BEGIN4(x / 10000000000000000); MIDDLE16(x % 10000000000000000); }
    *p = 0;

}

void i64toa(int64_t x, char* p)
{
    uint64_t t;
    if(x >= 0) t = x;
    else *p++ = '-', t = -(uint64_t)(x);
    u64toa(t, p);

}

int isEnglishName(const char *name)
{
    if (!name) {
        return 0;
    }
    for (int i = 0; name[i] != '\0'; i++) {
        if (!isalpha(name[i])) {
            return 0;
        }
    }
    return 1;
}

void utilFreeStrv(int argc, char **argv)
{
    while (--argc) free(argv[argc]);
}

size_t getFileSize(const char *filename)
{
    size_t return_value;
    FILE *fp = fopen(filename, "r");
    fseek(fp, 0, SEEK_END);
    return_value = (size_t)ftell(fp);
    return return_value;
}

size_t rewriteFile(const char *filename, const char *content)
{
    FILE *fp = fopen(filename, "w");
    size_t return_value = strlen(content);
    fwrite(content, sizeof(char), (size_t)return_value, fp);
    fclose(fp);
    return return_value;
}

char *getFileContent(const char *filename, size_t *size)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    *size = (size_t)ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *dest = stralloc((size_t)(*size) + 1);
    fread(dest, sizeof(char), (size_t)*size, fp);
    fclose(fp);
    return dest;
}
