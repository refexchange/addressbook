#include "util.h"
#include "crypto.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>

unsigned int global_seed = 0;

Payload *__hashcode = NULL;

void _init_hashcode()
{
    __hashcode = palloc(Payload);
    const static uint32_t hashcode[24] = {
        1382635611, 1059542905, 608844070, 1312842352,
        1798260027, 1582003309, 1920021044, 1397170552,
        694450758, 1446079613, 1866548330, 1228299322,
        577659182, 1043168096, 927083592, 2002017615,
        678765401, 1027368572, 1428512554, 894503258,
        1664109600, 1129336190, 1751460664, 1986491187
    };
    __hashcode->length = 24;
    __hashcode->seq = (CipherData*)calloc(24, sizeof(CipherData));
    memcpy(__hashcode->seq, hashcode, 96);
}

Payload *_get_hashcode()
{
    if (!__hashcode) _init_hashcode();
    return __hashcode;
}

Payload* payload_alloc()
{
    Payload *ret = palloc(Payload);
    ret->seq = NULL;
    ret->length = 0;
    return ret;
}

void free_payload(Payload *target)
{
    free(target->seq);
    free(target);
}

unsigned int safe_rand()
{
    if (global_seed == 0) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        long ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        global_seed = (unsigned)(ms % 196871);
    }
    global_seed = (214013 * (global_seed) + 2531011);
    return (global_seed >> 16) & 0x7FFF;
}

char* cryptoGenerateUUID()
{
    char *ret = stralloc(16);
    for (int i = 0; i < 15; i++) {
        int k = safe_rand() % 62;
        if (k < 26) ret[i] = (char)('A' + k);
        else if (k < 52) ret[i] = (char)('a' + k - 26);
        else ret[i] = (char)('0' + k - 52);
    }
    return ret;
}

Payload *payloadDecode(const char *source)
{
    if (!source) {
        return NULL;
    }
    size_t buffer_size = strlen(source);
    size_t cipher_size = (buffer_size - 1) / 4 + 1;
    Payload *cipher = palloc(Payload);
    cipher->seq = (CipherData*)calloc(cipher_size, sizeof(CipherData));
    cipher->length = cipher_size;
    for (int i = 0; i < buffer_size; i++) {
        int word_index = i / 4;
        int byte_index = i % 4;
        cipher->seq[word_index].bytes[byte_index] = source[i];
    }
    return cipher;
}

char *payloadEncode(const Payload *cipher)
{
    if (!cipher) {
        return NULL;
    }
    size_t buffer_size = cipher->length * 4 + 1;
    char *buffer = stralloc(buffer_size);
    for (int i = 0; i < buffer_size; i++) {
        int word_index = i / 4;
        int byte_index = i % 4;
        buffer[i] = cipher->seq[word_index].bytes[byte_index];
    }
    buffer[buffer_size - 1] = '\0';
    return buffer;
}

char *payload_digit_dumps(const Payload *cipher)
{
    if (!cipher) {
        return NULL;
    }
    size_t buffer_size = cipher->length * 10;
    char *buffer = stralloc(buffer_size + 1);
    for (int i = 0; i < cipher->length; i++) {
        uint32_t p = cipher->seq[i].word;
        for (int j = i * 10 + 9; j >= i * 10; j--) {
            buffer[j] = (char)((p % 10) + '0');
            p /= 10;
        }
    }
    return buffer;
}

Payload *XOREncrypt(const Payload *msg, const Payload *pwd)
{
    if (!msg || !pwd) return NULL;
    Payload *dest = palloc(Payload);
    dest->seq = (CipherData*)calloc(msg->length, sizeof(CipherData));
    dest->length = msg->length;
    for (int i = 0; i < msg->length; i++) {
        dest->seq[i].word = msg->seq[i].word;
        for (int j = 0; j < pwd->length; j++) {
            dest->seq[i].word ^= pwd->seq[j].word;
        }
    }
    return dest;
}

Payload *cryptoEncrypt(const Payload *content, const Payload *password)
{
    return XOREncrypt(content, password);
}

Payload *cryptoDecrypt(const Payload *cipher, const Payload *password)
{
    return XOREncrypt(cipher, password);
}

char* cryptoHash(const char *password)
{
    Payload *password_payload = payloadDecode(password);
    Payload *prehash = cryptoEncrypt(password_payload, _get_hashcode());
    char *hashed = payload_digit_dumps(prehash);
    free_payload(password_payload);
    free_payload(prehash);
    return hashed;
}

int cryptoVerifyHash(const char *password, const char *password_hash)
{
    char *hash = cryptoHash(password);
    int result = strcmp(password_hash, hash) == 0 ? YES : NO;
    free(hash);
    return result;
}

size_t cryptoWriteFile(const char *filename, const char *content, const char *password)
{
    Payload *message_payload = payloadDecode(content);
    Payload *password_payload = payloadDecode(password);
    Payload *cipher_payload = cryptoEncrypt(message_payload, password_payload);
    FILE *fp = fopen(filename, "wb");
    size_t i;
    for (i = 0; i < cipher_payload->length * 4; i++) {
        int word_index = (int)i / 4;
        int byte_index = (int)i % 4;
        fputc(cipher_payload->seq[word_index].bytes[byte_index], fp);
    }
    fclose(fp);
    free_payload(message_payload);
    free_payload(password_payload);
    free_payload(cipher_payload);
    return i;
}

char *cryptoLoadFile(const char *filename, const char *password)
{
    size_t file_size = getFileSize(filename);
    Payload *file_payload = palloc(Payload);
    file_payload->length = (file_size - 1) / 4 + 1;
    file_payload->seq = (CipherData*)calloc(file_payload->length, sizeof(CipherData));

    FILE *fp = fopen(filename, "rb");
    for (int i = 0; i < file_size; i++) {
        int word_index = i / 4;
        int byte_index = i % 4;
        file_payload->seq[word_index].bytes[byte_index] = (char)fgetc(fp);
    }
    fclose(fp);
    Payload *password_payload = payloadDecode(password);
    Payload *decrypted_payload = cryptoDecrypt(file_payload, password_payload);

    char *message = payloadEncode(decrypted_payload);
    free_payload(file_payload);
    free_payload(password_payload);
    free_payload(decrypted_payload);
    return message;
}