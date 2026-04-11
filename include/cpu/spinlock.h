#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "stddef.h"
#include "stdint.h"

typedef struct {
  int locked;
} spinlock_t;

void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);

#endif