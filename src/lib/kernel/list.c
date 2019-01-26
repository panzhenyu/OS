#include "list.h"
#include "interrupt.h"
#include "debug.h"

/* 初始化链表 */
void list_init(struct list* plist)
{
    plist->head.prev = NULL;
    plist->head.next = &plist->tail;
    plist->tail.prev = &plist->head;
    plist->tail.next = NULL;
}

/* 将链表元素elem插入在before之前 */
void list_insert_before(struct list_elem* before, struct list_elem* elem)
{
    enum intr_status old_status = intr_disable();
    // 不能在头结点之前插入元素
    if(before->prev != NULL)
    {
        elem->prev = before->prev;
        elem->next = before;
        before->prev->next = elem;
        before->prev = elem;
    }
    else
    {
        PANIC("cannot add element before list head");
    }
    intr_set_status(old_status);
}

/* 添加元素到链表队首 */
void list_push(struct list* plist, struct list_elem* elem)
{
    list_insert_before(plist->head.next, elem);
}

void list_iterate(const struct list* plist);

/* 追加元素到链表队尾*/
void list_append(struct list* plist, struct list_elem* elem)
{
    list_insert_before(&plist->tail, elem);
}

/* 删除元素elem */
void list_remove(const struct list_elem* elem)
{
    enum intr_status old_status = intr_disable();
    if(elem->prev == NULL || elem->next == NULL)
    {
        PANIC("cannot remove list head or tail");
    }
    else
    {
        elem->prev->next = elem->next;
        elem->next->prev = elem->prev;
        intr_enable(old_status);
    }
}

/* 弹出队首元素 */
struct list_elem* list_pop(struct list* plist)
{
    if(list_isEmpty(plist))
        return NULL;
    struct list_elem* elem = plist->head.next;
    list_remove(elem);
    return elem;
}

/* 判断链表是否为空 */
int list_isEmpty(const struct list* plist)
{
    return (plist->head.next == &plist->tail) ? 1 : 0;
}

/* 获取链表长度 */
uint32_t list_len(const struct list* plist)
{
    uint32_t len = 0;
    struct list_elem* p = plist->head.next;
    while(p != &plist->tail)
    {
        len++;
        p = p->next;
    }
    return len;
}

/* 
 * 把列表plist中的每个元素elem和arg传给回调函数func
 * arg给func用来判断elem是否符合条件
 * 遍历列表所有元素，逐个判断是否有符合条件的元素
 * 找到符合条件的元素指针并返回，否则返回NULL
 */
struct list_elem* list_traversal(struct list* plist, function func, int arg)
{
    if(list_isEmpty(plist))
        return NULL;
    struct list_elem* p = plist->head.next;
    while(p != &plist->tail)
    {
        if(func(p, arg))
            return p;
        p = p->next;
    }
    return NULL;
}

/* 判断链表是否含有某元素 */
int have_elem(const struct list* plist, const struct list_elem* elem)
{
    struct list_elem* p = plist->head.next;
    while(p != &plist->tail)
    {
        if(p == elem)
            return 1;
        p = p->next;
    }
    return 0;
}