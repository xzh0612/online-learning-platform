/*
 * link.c —— 公共双向链表实现（ModuleA：用户管理模块）
 *
 * 本文件实现 link_pub.h 中声明的接口，供本模块与 ModuleB（课程管理）、
 * ModuleC（数据录入与日志）共同使用。
 *
 * 实现要点：
 *   1. 带头结点：head 是哑结点，不存放业务数据；第一个数据节点是 head->next。
 *      这样在表头增删元素时无需修改调用方持有的头指针。
 *   2. 只管理节点自身的内存，void *data 指向的业务数据由调用方负责释放，
 *      释放动作通过 freefn 回调传入。
 */

#include "common.h"
#include "link_pub.h"

/* 创建带头结点的空链表 */
ListNode *list_create(void)
{
    ListNode *head = (ListNode *)malloc(sizeof(ListNode));

    if (head == NULL) {
        return NULL;
    }
    head->data = NULL;
    head->prev = NULL;
    head->next = NULL;
    return head;
}

/* 在链表尾部追加一个数据节点 */
int list_append(ListNode *head, void *data)
{
    ListNode *tail;
    ListNode *node;

    if (head == NULL) {
        return RES_ERR_PARAM;
    }
    node = (ListNode *)malloc(sizeof(ListNode));
    if (node == NULL) {
        return RES_ERR_MEMORY;
    }

    /* 找到最后一个数据节点 */
    tail = head;
    while (tail->next != NULL) {
        tail = tail->next;
    }

    node->data = data;
    node->prev = tail;
    node->next = NULL;
    tail->next = node;
    return RES_OK;
}

/* 在已有节点 node 之后插入新节点 */
int list_insert_after(ListNode *node, void *data)
{
    ListNode *nw;

    if (node == NULL) {
        return RES_ERR_PARAM;
    }
    nw = (ListNode *)malloc(sizeof(ListNode));
    if (nw == NULL) {
        return RES_ERR_MEMORY;
    }

    nw->data = data;
    nw->prev = node;
    nw->next = node->next;
    if (node->next != NULL) {
        node->next->prev = nw;
    }
    node->next = nw;
    return RES_OK;
}

/* 按比较函数查找数据节点 */
ListNode *list_find(ListNode *head,
                    int (*cmp)(const void *data, const void *key),
                    const void *key)
{
    ListNode *p;

    if (head == NULL || cmp == NULL) {
        return NULL;
    }
    for (p = head->next; p != NULL; p = p->next) {
        if (cmp(p->data, key) == 0) {
            return p;
        }
    }
    return NULL;
}

/* 取第 index 个数据节点（index 从 1 开始） */
ListNode *list_at(ListNode *head, int index)
{
    ListNode *p;
    int       i;

    if (head == NULL || index < 1) {
        return NULL;
    }
    p = head->next;
    for (i = 1; p != NULL && i < index; i++) {
        p = p->next;
    }
    return p;
}

/* 返回有效数据节点的个数 */
int list_count(ListNode *head)
{
    ListNode *p;
    int       n = 0;

    if (head == NULL) {
        return 0;
    }
    for (p = head->next; p != NULL; p = p->next) {
        n++;
    }
    return n;
}

/* 遍历链表，对每个数据节点执行 visit(data, arg) */
void list_foreach(ListNode *head, void (*visit)(void *data, void *arg), void *arg)
{
    ListNode *p;
    ListNode *next;

    if (head == NULL || visit == NULL) {
        return;
    }
    for (p = head->next; p != NULL; p = next) {
        next = p->next;
        visit(p->data, arg);
    }
}

/* 删除指定节点：仅释放节点自身，不释放 data */
int list_remove(ListNode *head, ListNode *node)
{
    if (head == NULL || node == NULL || node == head) {
        return RES_ERR_PARAM;
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    }

    free(node);
    return RES_OK;
}

/* 清空所有数据节点，保留头结点 */
void list_clear(ListNode *head, void (*freefn)(void *data))
{
    ListNode *p;
    ListNode *next;

    if (head == NULL) {
        return;
    }
    for (p = head->next; p != NULL; p = next) {
        next = p->next;
        if (freefn != NULL && p->data != NULL) {
            freefn(p->data);
        }
        free(p);
    }
    head->next = NULL;
}

/* 清空并释放头结点 */
void list_destroy(ListNode **phead, void (*freefn)(void *data))
{
    if (phead == NULL || *phead == NULL) {
        return;
    }
    list_clear(*phead, freefn);
    free(*phead);
    *phead = NULL;
}
