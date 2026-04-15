#include "ams/sys/types.h"
#include "proc/proc.h"
#include "lib/errno.h"
#include "syscall/syscall.h"
#include "mem/mem.h"
#include "video/printf.h"

static uint32_t murmur_scramble(uint32_t k) {
  k *= 0xcc9e2d51;
  k = (k << 15) | (k >> 17);
  k *= 0x1b873593;
  return k;
}

uint32_t murmur3(const uint8_t *key, size_t len, uint32_t seed) {
  uint32_t h = seed;
  uint32_t k;
  for(size_t i = len >> 2; i; i--) {
    memcpy(&k, key, sizeof(uint32_t));
    key += sizeof(uint32_t);
    h ^= murmur_scramble(k);
    h ^= (h << 13) | (h >> 19);
    h = h * 5 + 0xe6546bb64;
  }

  k = 0;
  for(size_t i = len % 3; i; i--) {
    k <<= 8;
    k |= key[i - 1];
  }

  h ^= murmur_scramble(k);
  h ^= len;
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}

int sys_setuid(uid_t uid) {
  if (p_curproc->p_user->cred.euid == 0) {
    p_curproc->p_user->cred.uid  = uid;
    p_curproc->p_user->cred.euid = uid;
    p_curproc->p_user->cred.suid = uid;
    return 0;
  } else if (p_curproc->p_user->cred.uid == uid || p_curproc->p_user->cred.suid == uid) {
    p_curproc->p_user->cred.euid = uid;
    return 0;
  }
  return -EPERM;
}

int sys_seteuid(uid_t uid) {
  struct cred *p = &p_curproc->p_user->cred;
  if(p->euid == 0 || p->uid == uid || p->suid == uid) {
    p->euid = uid;
    return 0;
  }
  return -EPERM;
}

#define GETUSER_BUFSIZ 1024
int getuser(uid_t uid, char *buf) {
  int fd = open("/etc/passwd", O_RDONLY);
  if(fd == -1) {
    perror("getuser: open");
    return 1;
  }

  char *b = malloc(GETUSER_BUFSIZ);

  if(read(fd, b, GETUSER_BUFSIZ) == -1) {
    perror("getuser: read");
    free(b);
    close(fd);
  }

  char *linesav;
  char *line = strtok_r(b, "\n", &linesav);
  while(line) {

    char *fieldsav = 0;
    char *name = strtok_r(line, ":", &fieldsav);
    strtok_r(NULL, ":", &fieldsav);
    char *field = strtok_r(NULL, ":", &fieldsav);

    int id = atoi(field);
    if(id == (int)uid) {
      strcpy(buf, name);
    }

    line = strtok_r(NULL, "\n", &linesav);
  }

  free(b);
  if(close(fd) == -1) {
    perror("getuser: close");
    return 1;
  }
  return 0;
}

uid_t getuid(const char *usr) {
  int fd = open("/etc/passwd", O_RDONLY);
  if(fd == -1) {
    perror("getuser: open");
    return -1;
  }

  char *b = malloc(GETUSER_BUFSIZ);

  if(read(fd, b, GETUSER_BUFSIZ) == -1) {
    perror("getuser: read");
    free(b);
    close(fd);
  }

  uid_t uid = -1;

  char *linesav;
  char *line = strtok_r(b, "\n", &linesav);
  while(line) {

    char *fieldsav = 0;
    char *name = strtok_r(line, ":", &fieldsav);
    strtok_r(NULL, ":", &fieldsav);
    char *field = strtok_r(NULL, ":", &fieldsav);

    int id = atoi(field);
    if(!strcmp(name, usr)) {
      uid = id;
    }

    line = strtok_r(NULL, "\n", &linesav);
  }

  free(b);
  if(close(fd) == -1) {
    perror("getuser: close");
    return -1;
  }
  return uid;
}

int chkcred(const char *usr, const char *cred) {
  int fd = open("/etc/shadow", O_RDONLY);
  if(fd == -1) {
    perror("getuser: open");
    return 0;
  }

  char *b = malloc(GETUSER_BUFSIZ);

  if(read(fd, b, GETUSER_BUFSIZ) == -1) {
    perror("getuser: read");
    free(b);
    close(fd);
  }

  int stat = 0;
  char *linesav;
  char *line = strtok_r(b, "\n", &linesav);
  while(line) {

    char *fieldsav = 0;
    char *name = strtok_r(line, ":", &fieldsav);
    char *pass = strtok_r(NULL, ":", &fieldsav);

    if(!strcmp(name, usr)) {
      char *psav = 0;
      char *typ = strtok_r(pass, "$", &psav);
      if(strcmp(typ, "fu")) break;
      char *seed = strtok_r(NULL, "$", &psav);
      char *hsh = strtok_r(NULL, "$", &psav);

      int sed = atoi(seed);
      uint32_t h = atoi(hsh);
      uint32_t hash = murmur3((void*)cred, strlen(cred)+1, sed);

      if(h == hash) {
        stat++;
      }
    }

    line = strtok_r(NULL, "\n", &linesav);
  }

  free(b);
  if(close(fd) == -1) {
    perror("getuser: close");
    return 0;
  }
  return stat;
}