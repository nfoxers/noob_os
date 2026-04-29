#ifndef CRYPT_H
#define CRYPT_H

#include "stddef.h"
#include "stdint.h"
#include "proc/proc.h"

uint32_t murmur3(const uint8_t *key, size_t len, uint32_t seed);

int getuser(const char *name, uid_t uid, struct userinfo *info);
int chkcred(const char *usr, const char *cred, struct userinfo *info);

/* rand.c */

void insert_ent(uint32_t ent);
void get_rand(uint8_t *out, size_t outlen);
void get_entpool(uint8_t out[static 32]);

#endif