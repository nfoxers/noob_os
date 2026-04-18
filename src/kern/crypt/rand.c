#include "crypt/blake2s.h"
#include "crypt/crypt.h"
#include "driver/time.h"
#include "mem/mem.h"

static uint8_t ent_pool[32];

void rand_init() {
  uint32_t t = 0;
  uint32_t m = 0;
  t = gettime(&m);
  t = ((t * 10000 + m) * 426767 - 2786) ^ (t << 5 | m );
  blake2s(ent_pool, 32, &t, 4, &t, 4);
}

void insert_ent(uint32_t ent) {
  uint8_t pool2[32];
  memcpy(pool2, ent_pool, 32);
  *(uint32_t *)pool2 = ent;

  blake2s(ent_pool, 32, &ent, 4, pool2, 32);
}

void get_rand(uint8_t *out, size_t outlen) {
  if(!out || outlen == 0) return;
  blake2s(out, outlen, out, 0, ent_pool, 32);

  uint8_t pool2[32];
  memcpy(pool2, ent_pool, 32);
  *(uint32_t *)pool2 = (uintptr_t)out;

  blake2s(ent_pool, 32, out, 0, pool2, 32);
}