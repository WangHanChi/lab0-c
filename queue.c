#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */


/* Create an empty queue */
struct list_head *q_new()
{
    struct list_head *empty = malloc(sizeof(struct list_head));
    if (!empty)
        return NULL;
    INIT_LIST_HEAD(empty);
    return empty;
}

/* Free all storage used by queue */
void q_free(struct list_head *l)
{
    if (list_empty(l))
        return;
    struct list_head *iter = l;
    element_t *node, *node_safe;
    list_for_each_entry_safe (node, node_safe, iter, list) {
        list_del(&node->list);
        q_release_element(node);
    }
    free(l);
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *node = malloc(sizeof(element_t));
    if (!node)
        return false;
    int str_size = strlen(s) * sizeof(char);
    char *str = malloc(str_size + 1);
    if (!str) {
        free(node);
        return false;
    }
    strncpy(str, s, str_size);
    str[str_size] = '\0';
    node->value = str;
    list_add(&node->list, head);
    return true;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *node = malloc(sizeof(element_t));
    if (!node)
        return false;
    int str_size = strlen(s) * sizeof(char);
    char *str = malloc(str_size + 1);
    if (!str) {
        free(node);
        return false;
    }
    strncpy(str, s, str_size);
    str[str_size] = '\0';
    node->value = str;
    list_add_tail(&node->list, head);
    return true;
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    element_t *remove_node = list_entry(head->next, element_t, list);
    if (bufsize) {
        strncpy(sp, remove_node->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }
    list_del(&remove_node->list);
    return remove_node;
}

/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    element_t *remove_node = list_entry(head->prev, element_t, list);
    if (bufsize) {
        strncpy(sp, remove_node->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }
    list_del(&remove_node->list);
    return remove_node;
}

/* Return number of elements in queue */
int q_size(struct list_head *head)
{
    if (!head || list_empty(head))
        return 0;
    int count = 0;
    struct list_head *node;
    list_for_each (node, head)
        ++count;
    return count;
}

/* Delete the middle node in queue */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/

    if (!head || list_empty(head))
        return false;
    struct list_head *rabbit = head, *turtle = head;
    do {
        rabbit = rabbit->next->next;
        if (rabbit == head->prev || rabbit == head) {
            break;
        }
        turtle = turtle->next;
    } while (!(rabbit == head->prev) && !(rabbit == head));

    element_t *node = list_entry(turtle->next, element_t, list);
    list_del(&node->list);
    q_release_element(node);
    return true;
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/

    if (!head)
        return false;
    if (list_empty(head) || list_is_singular(head))
        return true;
    element_t *node, *safe;
    bool flag = 0;
    list_for_each_entry_safe (node, safe, head, list) {
        char *str = list_entry(node->list.next, element_t, list)->value;
        if (node->list.next != head && !strcmp(str, node->value)) {
            list_del(&node->list);
            q_release_element(node);
            flag = 1;
        } else if (flag) {
            list_del(&node->list);
            q_release_element(node);
            flag = 0;
        }
    }
    return true;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    // https://leetcode.com/problems/swap-nodes-in-pairs/

    if (!head || list_is_singular(head))
        return;
    struct list_head *node, *next;
    for (node = head->next, next = node->next; node != head && next != head;
         node = node->next, next = node->next) {
        list_del(next);
        next->prev = node->prev;
        next->next = node;
        node->prev->next = next;
        node->prev = next;
    }
}

/* Reverse elements in queue */
void q_reverse(struct list_head *head)
{
    if (!head || list_empty(head))
        return;
    struct list_head *before = head->prev, *current = head, *after = NULL;
    while (after != head) {
        after = current->next;
        current->next = before;
        current->prev = after;
        before = current;
        current = after;
    }
}

/* Reverse the nodes of the list k at a time */
void q_reverseK(struct list_head *head, int k)
{
    // https://leetcode.com/problems/reverse-nodes-in-k-group/

    if (!head || list_empty(head) || list_is_singular(head))
        return;

    struct list_head *node, *safe, *tmp = head;
    LIST_HEAD(new_head);

    int num = 0;
    list_for_each_safe (node, safe, head) {
        ++num;
        if (num == k) {
            list_cut_position(&new_head, tmp, node);
            q_reverse(&new_head);
            list_splice_init(&new_head, tmp);
            num = 0;
            tmp = safe->prev;
        }
    }
}

/* Sort elements of queue in ascending order */
void q_sort(struct list_head *head) {}

/* Remove every node which has a node with a strictly greater value anywhere to
 * the right side of it */
int q_descend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    return 0;
}

/* Merge all the queues into one sorted queue, which is in ascending order */
int q_merge(struct list_head *head)
{
    // https://leetcode.com/problems/merge-k-sorted-lists/
    return 0;
}
