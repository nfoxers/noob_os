void tmain(void *arg) {
  (void)arg;

  while(1)
    *(volatile char *)0xb8000 = 'A';
}