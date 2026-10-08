#ifndef _LINK_PUB_H_
#define _LINK_PUB_H_

/*
 * link_pub.h —— 公共双向链表对外接口
 *
 * 由 ModuleA（用户管理模块）实现，公开给 ModuleB（课程管理）与
 * ModuleC（数据录入与日志）调用。课程管理模块在维护「已选学生列表」时，
 * 需要通过本接口完成往链表中添加节点、查找节点等操作。
 *
 * 设计约定：
 *   1. 采用「带头结点」的双向链表：head 本身不存放业务数据。
 *      这样在头部插入/删除元素时无需修改调用方持有的头指针，接口更稳定。
 *   2. 节点只持有业务数据的指针（void *data），链表本身不负责 data 的生命周期，
 *      释放动作由调用方通过 freefn 回调指定。
 */

#include "common.h"

typedef struct ListNode {
    void            *data;
    struct ListNode *prev;
    struct ListNode *next;
} ListNode;

/* 创建带头结点的空链表，失败返回 NULL */
ListNode *list_create(void);

/* 在链表尾部追加一个数据节点；成功返回 RES_OK */
int list_append(ListNode *head, void *data);

/* 在已有节点 node 之后插入新节点；成功返回 RES_OK */
int list_insert_after(ListNode *node, void *data);

/*
 * 查找数据节点：对每个节点执行 cmp(node->data, key)，
 * 返回值为 0 表示命中。未命中返回 NULL。
 */
ListNode *list_find(ListNode *head,
                    int (*cmp)(const void *data, const void *key),
                    const void *key);

/* 删除指定节点：仅释放节点本身，不释放 data；成功返回 RES_OK */
int list_remove(ListNode *head, ListNode *node);

/* 取第 index 个数据节点（index 从 1 开始），越界返回 NULL */
ListNode *list_at(ListNode *head, int index);

/* 返回有效数据节点的个数 */
int list_count(ListNode *head);

/* 遍历链表，对每个数据节点执行 visit(data, arg) */
void list_foreach(ListNode *head, void (*visit)(void *data, void *arg), void *arg);

/* 清空所有数据节点（对每个 data 调用 freefn，可为 NULL），保留头结点 */
void list_clear(ListNode *head, void (*freefn)(void *data));

/* 清空并释放头结点，同时把 *phead 置为 NULL */
void list_destroy(ListNode **phead, void (*freefn)(void *data));

#endif /* _LINK_PUB_H_ */
