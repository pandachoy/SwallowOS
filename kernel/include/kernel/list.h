#ifndef _LIST_H
#define _LIST_H

#include <stdbool.h>

#define LIST_POISON1              ((void*) 0x100)
#define LIST_POISON2              ((void*) 0x200)

#define container_of(ptr, type, member) \
    (type *)((char*)(ptr) - (char *)&((type *)0)->member)

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos !=(head); pos = pos->next)

#define is_last_entry(h) ((h)->next==(h))

struct list_head {
    struct list_head *next, *prev;
};

static void INIT_LIST_HEAD(struct list_head *list) {
    list->next = list;
    list->prev = list;
}

static bool __list_add_valid(struct list_head *new __attribute__((unused)),
                                    struct list_head *prev __attribute__((unused)),
                                    struct list_head *next __attribute__((unused))) {
    return true;
}

/* insert a new entry before the specified head, useful for queues */
static void __list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next) {
    if (!__list_add_valid(new, prev, next))
        return;

    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

/* insert new entry after specified head, useful for stacks */
static void list_add(struct list_head *new, struct list_head *head) {
    __list_add(new, head, head->next);
}

static void list_add_tail(struct list_head *new, struct list_head *head) {
    __list_add(new, head->prev, head);
}

static bool __list_del_entry_valid(struct list_head *entry __attribute__((unused))) {
    return true;
}

/* delete a list entry by making the prev/next entries */
static void __list_del(struct list_head *prev, struct list_head *next) {
    next->prev = prev;
    prev->next = next;
}

static void __list_del_entry(struct list_head *entry) {
    if (!__list_del_entry_valid(entry))
        return;
    __list_del(entry->prev, entry->next);
}

static void list_del(struct list_head *entry) {
    __list_del_entry(entry);
    entry->next = LIST_POISON1;
    entry->prev = LIST_POISON2;
}

static void list_replace(struct list_head *old, struct list_head *new) {
    new->next = old->next;
    new->next->prev = new;
    new->prev = old->prev;
    new->prev->next = new;
}

static void list_replace_init(struct list_head *old, struct list_head *new) {
    list_replace(old, new);
    INIT_LIST_HEAD(old);
}

#endif