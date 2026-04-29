#include <fs/vfs.h>
#include <crypt/crypt.h>
#include <mem/mem.h>
#include <video/printf.h>

#define HITAB_MAX 64

struct hlist_head hitab[HITAB_MAX];
struct inode      ilru = {.dev = 99, .sb = 0, .lru.next = &ilru.lru, .lru.prev = &ilru.lru};

uint32_t ihash(dev_t dev, ino_t ino) {
  uint32_t k = murmur3((uint8_t[]){dev, ino}, 2, dev + ino);
  return k % HITAB_MAX;
}

struct inode *inode_alloc(struct super_block *sb) {
  struct inode *in = malloc(sizeof(struct inode));
  in->dev          = sb->s_dev;
  return in;
}

void iadd(struct inode *in) {
  uint32_t h = ihash(in->sb->s_dev, in->ino);
  hlist_add_head(&in->hnode, &hitab[h]);
}

struct inode *iget(struct super_block *sb, ino_t fsino) {
  if (!sb)
    return NULL;

  uint32_t h = ihash(sb->s_dev, fsino);
  struct hlist_node *k = hitab[h].first;

  while (k) {
    struct inode *in = container_of(k, struct inode, hnode);
    if ((in->sb->s_dev == sb->s_dev) && (in->ino == fsino)) {
      in->refs++;
      return in;
    }
    k = k->next;
  }

  // ok, maybe the inode isnt inside the hlist table, try the lru
  struct list_head *m = &ilru.lru;
  do {
    struct inode *in = container_of(m, struct inode, lru);
    if (in->sb == 0) {
      m = m->next;
      continue;
    }
    if ((in->sb->s_dev == sb->s_dev) && (in->ino == fsino)) {
      if (in->refs == 0)
        list_del(&in->lru);
      in->refs++;
      hlist_add_head(&in->hnode, &hitab[h]);
      return in;
    }
    m = m->next;
  } while (m != &ilru.lru);

  // printkf("cache miss (all)\n");
  struct inode *in = malloc(sizeof(struct inode));
  in->ino          = fsino;
  in->dev          = sb->s_dev;
  in->sb           = sb;
  in->refs         = 1;
  init_list(&in->lru);

  if (sb->s_op && sb->s_op->read_inode)
    sb->s_op->read_inode(in);

  in->ino  = fsino;
  in->dev  = sb->s_dev;
  in->sb   = sb;
  in->refs = 1;

  hlist_add_head(&in->hnode, &hitab[h]);
  return in;
}

void free_inode(struct inode *in) {
  if (in->sb && in->sb->s_op && in->sb->s_op->put_inode)
    in->sb->s_op->put_inode(in);
  list_del(&in->lru);
  free(in);
}

void iput(struct inode *in) {
  if (in->refs == 0) {
    printkf("err: inode has 0 refs at iput\n");
    return;
  }

  in->refs--;

  if (in->refs > 0)
    return;

  if (in->iflags & IN_DIRTY) {
    if (in->sb->s_op->write_inode)
      in->sb->s_op->write_inode(0, in);
  }

  list_add(&in->lru, &ilru.lru);
  hlist_del(&in->hnode);
}

void imount(struct inode *in, struct mount *mnt) {
  in->iflags |= IN_MOUNT;
  in->mnt = mnt;

  mnt->mountpt = in;
  
}

void purge_lru() {
  struct list_head *m = &ilru.lru;
  while (m->next != m) {
    struct list_head *k  = m->next;
    struct inode     *in = container_of(k, struct inode, lru);
    free_inode(in);
  }
}

void print_caches() {
  int tmp = 0;
  printkf("HASH CACHE:\n");
  for (int h = 0; h < HITAB_MAX; h++) {
    tmp                  = 0;
    struct hlist_node *k = hitab[h].first;
    if (k) {
      printkf("[%02x]: ", h);
      tmp = 1;
    }
    while (k) {
      struct inode *in = container_of(k, struct inode, hnode);
      printkf("%d@%d.", in->ino, in->sb->s_dev);
      printkf("%d ", in->refs);
      k = k->next;
    }
    if (tmp)
      printkf("\n");
  }
  printkf("\nLRU CACHE:\n");
  struct list_head *m = &ilru.lru;
  do {
    struct inode *in = container_of(m, struct inode, lru);
    printkf("[%p]: %d@%d.%d\n", in, in->ino, in->dev, in->refs);
    m = m->next;
  } while (m != &ilru.lru);

  printkf("\nFILE DESCS\n");
  for (size_t i = 0; i < sizeof(p_curproc->p_user->u_ofile) / sizeof(p_curproc->p_user->u_ofile[0]); i++) {
    if (p_curproc->p_user->u_ofile[i]) {
      printkf("[%02d]: %p.%d\n", i, p_curproc->p_user->u_ofile[i]->inode, p_curproc->p_user->u_ofile[i]->refcont);
    }
  }
}