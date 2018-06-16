//
// Created by refexchange on 2018/5/8.
//

#ifndef CONTACTS_CRYPTO_H
#define CONTACTS_CRYPTO_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef uint32_t uint32;

typedef union {
    char bytes[4];
    uint32 word;
} CipherData;

typedef struct
{
    CipherData *seq;
    size_t length;
} Payload;

Payload *payloadDecode(const char *source);
char *payloadEncode(const Payload *cipher);

char* cryptoGenerateUUID();
Payload *cryptoEncrypt(const Payload *content, const Payload *password);
Payload *cryptoDecrypt(const Payload *cipher, const Payload *password);

size_t cryptoWriteFile(const char *filename, const char *content, const char *password);
char *cryptoLoadFile(const char *filename, const char *password);

char* cryptoHash(const char *password);
int cryptoVerifyHash(const char *password, const char *password_hash);

#endif //CONTACTS_CRYPTO_H