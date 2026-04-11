#include "stdint.h"
#include "cpu/spinlock.h"

void spin_lock(spinlock_t *lock) {
  while(__atomic_exchange_n(&lock->locked, 1, __ATOMIC_ACQUIRE)) {
    while(__atomic_load_n(&lock->locked, __ATOMIC_RELAXED)) {
      asm volatile("pause");
    }
  }
}

void spin_unlock(spinlock_t *lock) {
  __atomic_store_n(&lock->locked, 0, __ATOMIC_RELEASE);
}

static inline uint32_t irq_save() {
  uint32_t f;
  asm volatile(
    "pushf\n"
    "pop %0\n"
    "cli"
    : "=r"(f)
    :: "memory"
  );
  return f;
}

static inline void irq_restore(uint32_t f) {
  asm volatile(
    "push %0\n"
    "popf\n"
    :: "r"(f)
    : "memory", "cc"
  );
}

void spin_lock_irqsave(spinlock_t *lock, uint32_t *f) {
  *f = irq_save();
  spin_lock(lock);
}

void spin_lock_irqrestore(spinlock_t *lock, uint32_t *f) {
  spin_unlock(lock);
  irq_restore(*f);
}