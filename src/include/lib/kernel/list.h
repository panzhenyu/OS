#ifndef _LIB_KERNEL_LIST_H
#define _LIB_KERNEL_LIST_H

#include "global.h"

struct list_elem
{
    struct list_elem* prev;
    struct list_elem* next;
};

struct list
{
    struct list_elem head;
    struct list_elem tail;
};

#define offset(struct_type, member) (uint32_t)(&((struct_type*)0)->member)
#define elem2entry(struct_type, struct_member, elem_ptr) (struct_type*)((int)elem_ptr - offset(struct_type, struct_member))
typedef int (function) (struct list_elem*, int arg);

void list_init(struct list* plist);
void list_insert_before(struct list_elem* before, struct list_elem* elem);
void list_push(struct list* plist, struct list_elem* elem);
void list_iterate(const struct list* plist);
void list_append(struct list* plist, struct list_elem* elem);
void list_remove(const struct list_elem* elem);
struct list_elem* list_pop(struct list* plist);
int list_isEmpty(const struct list* plist);
uint32_t list_len(const struct list* plist);
struct list_elem* list_traversal(struct list* plist, function func, int arg);
int have_elem(const struct list* plist, const struct list_elem* elem);

#endif