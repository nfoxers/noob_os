#ifndef LIST_H
#define LIST_H

struct list_head {
  struct list_head *next;
  struct list_head *prev;
};

struct hlist_node {
  struct hlist_node *next;
  struct hlist_node **pprev;
};

struct hlist_head {
  struct hlist_node *first;
};

#define container_of(ptr, type, member) \
  ((type *)((char *)(ptr) - offsetof(type, member)))

void init_list(struct list_head *head);
void list_add(struct list_head *new, struct list_head *head);
void list_add_tail(struct list_head *new, struct list_head *head);
void list_del(struct list_head *ent);

void hlist_add_head(struct hlist_node *new, struct hlist_head *head);
void hlist_add_tail(struct hlist_node *new, struct hlist_head *head);
void hlist_del(struct hlist_node *node);

#endif