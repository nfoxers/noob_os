#include "lib/list.h"
#include "stddef.h"

void init_list(struct list_head *head) {
  head->next = head;
  head->prev = head;
}

void list_add(struct list_head *new, struct list_head *head) {
  new->next = head->next;
  new->prev = head;

  head->next->prev = new;
  head->next = new;
}

void list_add_tail(struct list_head *new, struct list_head *head) {
  new->next = head;
  new->prev = head->prev;

  head->prev->next = new;
  head->prev = new;
}

void list_del(struct list_head *ent) {
  ent->prev->next = ent->next;
  ent->next->prev = ent->prev;

  ent->next = ent;
  ent->prev = ent;
}

void hlist_add_head(struct hlist_node *new, struct hlist_head *head) {
  new->next = head->first;
  new->pprev = &head->first;
  if(new->next) new->next->pprev = &new->next;
  head->first = new;
}

void hlist_add_tail(struct hlist_node *new, struct hlist_head *head) {
  struct hlist_node *cur;
  new->next = NULL;
  new->pprev = NULL;

  cur = head->first;
  while(cur->next) {
    cur = cur->next;
  }
  cur->next = new;
  new->pprev = &cur->next;
}

void hlist_del(struct hlist_node *node) {
  if(node->next) {
    node->next->pprev = node->pprev;
  }
  *(node->pprev) = node->next;
}