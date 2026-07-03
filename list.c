/*********************************************************************
 * 文件名：list.c
 * 说明：  双向链表实现（头插法），O(1) 插入。
 *         保留 prev/next 指针，支持双向遍历。
 *********************************************************************/

#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ListNode {
    EnrollmentRecord data;
    struct ListNode *prev, *next;
} ListNode;

typedef struct {
    ListNode *head;    // 仅头指针，无尾指针
    int size;
} LinkedList;

// 创建新节点
static ListNode* create_node(EnrollmentRecord rec) {
    ListNode *node = (ListNode*)malloc(sizeof(ListNode));
    node->data = rec;
    node->prev = node->next = NULL;
    return node;
}

// 头插法（O(1)）
static void list_insert(void *storage, EnrollmentRecord rec) {
    LinkedList *list = (LinkedList*)storage;
    ListNode *node = create_node(rec);
    if (list->head) {
        node->next = list->head;
        list->head->prev = node;
    }
    list->head = node;
    list->size++;
}

static void list_modify_score(void *storage, const char *sid, const char *cid, int score) {
    LinkedList *list = (LinkedList*)storage;
    ListNode *cur = list->head;
    while (cur) {
        if (strcmp(cur->data.student_id, sid) == 0 && strcmp(cur->data.course_id, cid) == 0) {
            cur->data.score = score;
            return;
        }
        cur = cur->next;
    }
}

static EnrollmentRecord* list_search(void *storage, const char *sid, const char *cid, int *count) {
    LinkedList *list = (LinkedList*)storage;
    ListNode *cur = list->head;
    int cnt = 0;
    while (cur) {
        if (strcmp(cur->data.student_id, sid) == 0 && strcmp(cur->data.course_id, cid) == 0)
            cnt++;
        cur = cur->next;
    }
    if (cnt == 0) { *count = 0; return NULL; }
    EnrollmentRecord *res = (EnrollmentRecord*)malloc(sizeof(EnrollmentRecord) * cnt);
    cur = list->head;
    int idx = 0;
    while (cur) {
        if (strcmp(cur->data.student_id, sid) == 0 && strcmp(cur->data.course_id, cid) == 0)
            res[idx++] = cur->data;
        cur = cur->next;
    }
    *count = cnt;
    return res;
}

static int list_delete(void *storage, const char *sid, const char *cid) {
    LinkedList *list = (LinkedList*)storage;
    ListNode *cur = list->head;
    while (cur) {
        if (strcmp(cur->data.student_id, sid) == 0 && strcmp(cur->data.course_id, cid) == 0) {
            if (cur->prev) cur->prev->next = cur->next;
            else list->head = cur->next;
            if (cur->next) cur->next->prev = cur->prev;
            free(cur);
            list->size--;
            return 1;
        }
        cur = cur->next;
    }
    return 0;
}

static void list_list_all(void *storage) {
    LinkedList *list = (LinkedList*)storage;
    ListNode *cur = list->head;
    int cnt = 0;
    printf("%-12s %-8s %-25s %-8s %-25s %4s %7s %10s %4s\n",
           "学号", "姓名", "学院", "课程编号", "课程名", "学分", "学期", "日期", "成绩");
    while (cur && cnt < 20) {
        printf("%-12s %-8s %-25s %-8s %-25s %4.1f %7s %4d-%02d-%02d %4d\n",
               cur->data.student_id, cur->data.name, cur->data.college,
               cur->data.course_id, cur->data.course_name,
               cur->data.credit, cur->data.semester,
               cur->data.year, cur->data.month, cur->data.day,
               cur->data.score);
        cur = cur->next;
        cnt++;
    }
    if (list->size > 20) printf("... (共 %d 条，仅显示前20条)\n", list->size);
}

static void list_get_all(void *storage, EnrollmentRecord **arr, int *size) {
    LinkedList *list = (LinkedList*)storage;
    *size = list->size;
    if (*size == 0) { *arr = NULL; return; }
    *arr = (EnrollmentRecord*)malloc(sizeof(EnrollmentRecord) * (*size));
    ListNode *cur = list->head;
    int i = 0;
    while (cur) {
        (*arr)[i++] = cur->data;
        cur = cur->next;
    }
}

static void list_clean_expired(void *storage, int before_year) {
    LinkedList *list = (LinkedList*)storage;
    ListNode *cur = list->head, *next;
    while (cur) {
        next = cur->next;
        if (cur->data.year < before_year) {
            if (cur->prev) cur->prev->next = cur->next;
            else list->head = cur->next;
            if (cur->next) cur->next->prev = cur->prev;
            free(cur);
            list->size--;
        }
        cur = next;
    }
}

static void list_destroy(void *storage) {
    LinkedList *list = (LinkedList*)storage;
    ListNode *cur = list->head;
    while (cur) {
        ListNode *tmp = cur;
        cur = cur->next;
        free(tmp);
    }
    list->head = NULL;
    list->size = 0;
}

void list_attach(DataStructure *ds) {
    LinkedList *list = (LinkedList*)malloc(sizeof(LinkedList));
    list->head = NULL;
    list->size = 0;
    ds->storage = list;
    ds->insert = list_insert;
    ds->modify_score = list_modify_score;
    ds->search = list_search;
    ds->delete = list_delete;
    ds->list_all = list_list_all;
    ds->get_all_records = list_get_all;
    ds->clean_expired = list_clean_expired;
    ds->destroy = list_destroy;
}
