#ifndef __LIB_KERNEL_LIST_H
#define __LIB_KERNEL_LIST_H
#include "global.h"

#define offset(struct_type, member) (int)(&((struct_type*)0)->member)
#define elem2entry(struct_type, struct_member_name, elem_ptr) \
            (struct_type*)((int)elem_ptr - offset(struct_type, struct_member_name))

/* 链表结点成员结构 */
struct list_elem {
    struct list_elem* prev; //前驱结点
    struct list_elem* next; //后继结点
};

/* 链表结构 */
struct list {
    //head 队首，固定不变，第 1 个元素为 head.next
    struct list_elem head;
    //tail 队尾，固定不变
    struct list_elem tail;
};

/* 自定义函数类型 function，用于在 list_traversal 中做回调函数 */
typedef int (function)(struct list_elem*, int arg);

void list_init(struct list* list);
void list_insert_before(struct list_elem* before, struct list_elem* elem);
void list_push(struct list* list, struct list_elem* elem);
void list_append(struct list* list, struct list_elem* elem);
void list_remove(struct list_elem* elem);
struct list_elem* list_pop(struct list* list);
int elem_find(struct list* list, struct list_elem* elem);
int list_empty(struct list* list);
uint32_t list_len(struct list* list);
struct list_elem* list_traversal(struct list* list, function func, int arg);;

#endif
