/*********************************************************************
 * 文件名：hash.c
 * 说明：  哈希表（链地址法），支持动态扩容，保持 O(1) 平均操作。
 *********************************************************************/

#include "hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_SIZE 53          // 初始桶数
#define LOAD_FACTOR 0.75         // 装载因子阈值

typedef struct HashNode {
    EnrollmentRecord data;
    struct HashNode *next;
} HashNode;

typedef struct {
    HashNode **buckets;
    int bucket_count;           // 当前桶数量
    int size;
} HashTable;

// DJB2 哈希（与之前相同）
static int hash_func(const char *sid, const char *cid, int bucket_count) {
    int h = 0;
    while (*sid) h = h * 31 + *sid++;
    while (*cid) h = h * 31 + *cid++;
    if (h < 0) h = -h;
    return h % bucket_count;
}

// 扩容：桶数翻倍，重新散列所有节点
static void hash_resize(HashTable *ht) {
    int new_bucket_count = ht->bucket_count * 2 + 1;   // 奇数容量
    HashNode **new_buckets = (HashNode**)calloc(new_bucket_count, sizeof(HashNode*));

    // 将旧桶中的所有节点重新插入到新桶
    for (int i = 0; i < ht->bucket_count; i++) {
        HashNode *cur = ht->buckets[i];
        while (cur) {
            HashNode *next = cur->next;
            int idx = hash_func(cur->data.student_id, cur->data.course_id, new_bucket_count);
            cur->next = new_buckets[idx];
            new_buckets[idx] = cur;
            cur = next;
        }
    }

    free(ht->buckets);
    ht->buckets = new_buckets;
    ht->bucket_count = new_bucket_count;
}

static void hash_insert(void *storage, EnrollmentRecord rec) {
    HashTable *ht = (HashTable*)storage;

    // 检查是否需要扩容
    if ((double)ht->size / ht->bucket_count >= LOAD_FACTOR) {
        hash_resize(ht);
    }

    int idx = hash_func(rec.student_id, rec.course_id, ht->bucket_count);
    HashNode *node = (HashNode*)malloc(sizeof(HashNode));
    node->data = rec;
    node->next = ht->buckets[idx];
    ht->buckets[idx] = node;
    ht->size++;
}

static void hash_modify_score(void *storage, const char *sid, const char *cid, int score) {
    HashTable *ht = (HashTable*)storage;
    int idx = hash_func(sid, cid, ht->bucket_count);
    HashNode *cur = ht->buckets[idx];
    while (cur) {
        if (strcmp(cur->data.student_id, sid) == 0 && strcmp(cur->data.course_id, cid) == 0) {
            cur->data.score = score;
            return;
        }
        cur = cur->next;
    }
}

static EnrollmentRecord* hash_search(void *storage, const char *sid, const char *cid, int *count) {
    HashTable *ht = (HashTable*)storage;
    int idx = hash_func(sid, cid, ht->bucket_count);
    HashNode *cur = ht->buckets[idx];
    int cnt = 0;
    while (cur) {
        if (strcmp(cur->data.student_id, sid) == 0 && strcmp(cur->data.course_id, cid) == 0)
            cnt++;
        cur = cur->next;
    }
    if (cnt == 0) { *count = 0; return NULL; }
    EnrollmentRecord *res = (EnrollmentRecord*)malloc(sizeof(EnrollmentRecord) * cnt);
    cur = ht->buckets[idx];
    int i = 0;
    while (cur) {
        if (strcmp(cur->data.student_id, sid) == 0 && strcmp(cur->data.course_id, cid) == 0)
            res[i++] = cur->data;
        cur = cur->next;
    }
    *count = cnt;
    return res;
}

static int hash_delete(void *storage, const char *sid, const char *cid) {
    HashTable *ht = (HashTable*)storage;
    int idx = hash_func(sid, cid, ht->bucket_count);
    HashNode *cur = ht->buckets[idx], *prev = NULL;
    while (cur) {
        if (strcmp(cur->data.student_id, sid) == 0 && strcmp(cur->data.course_id, cid) == 0) {
            if (prev) prev->next = cur->next;
            else ht->buckets[idx] = cur->next;
            free(cur);
            ht->size--;
            return 1;
        }
        prev = cur;
        cur = cur->next;
    }
    return 0;
}

static void hash_list_all(void *storage) {
    HashTable *ht = (HashTable*)storage;
    int cnt = 0;
    printf("%-12s %-8s %-25s %-8s %-25s %4s %7s %10s %4s\n",
           "学号", "姓名", "学院", "课程编号", "课程名", "学分", "学期", "日期", "成绩");
    for (int i = 0; i < ht->bucket_count && cnt < 20; i++) {
        HashNode *cur = ht->buckets[i];
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
    }
    if (ht->size > 20) printf("... (共 %d 条，仅显示前20条)\n", ht->size);
}

static void hash_get_all(void *storage, EnrollmentRecord **arr, int *size) {
    HashTable *ht = (HashTable*)storage;
    *size = ht->size;
    if (*size == 0) { *arr = NULL; return; }
    *arr = (EnrollmentRecord*)malloc(sizeof(EnrollmentRecord) * (*size));
    int idx = 0;
    for (int i = 0; i < ht->bucket_count; i++) {
        HashNode *cur = ht->buckets[i];
        while (cur) {
            (*arr)[idx++] = cur->data;
            cur = cur->next;
        }
    }
}

static void hash_clean_expired(void *storage, int before_year) {
    HashTable *ht = (HashTable*)storage;
    for (int i = 0; i < ht->bucket_count; i++) {
        HashNode *cur = ht->buckets[i], *prev = NULL;
        while (cur) {
            if (cur->data.year < before_year) {
                HashNode *tmp = cur;
                if (prev) prev->next = cur->next;
                else ht->buckets[i] = cur->next;
                cur = cur->next;
                free(tmp);
                ht->size--;
            } else {
                prev = cur;
                cur = cur->next;
            }
        }
    }
}

static void hash_destroy(void *storage) {
    HashTable *ht = (HashTable*)storage;
    for (int i = 0; i < ht->bucket_count; i++) {
        HashNode *cur = ht->buckets[i];
        while (cur) {
            HashNode *tmp = cur;
            cur = cur->next;
            free(tmp);
        }
    }
    free(ht->buckets);
    ht->buckets = NULL;
    ht->bucket_count = 0;
    ht->size = 0;
}

void hash_attach(DataStructure *ds) {
    HashTable *ht = (HashTable*)malloc(sizeof(HashTable));
    ht->bucket_count = INITIAL_SIZE;
    ht->buckets = (HashNode**)calloc(ht->bucket_count, sizeof(HashNode*));
    ht->size = 0;
    ds->storage = ht;
    ds->insert = hash_insert;
    ds->modify_score = hash_modify_score;
    ds->search = hash_search;
    ds->delete = hash_delete;
    ds->list_all = hash_list_all;
    ds->get_all_records = hash_get_all;
    ds->clean_expired = hash_clean_expired;
    ds->destroy = hash_destroy;
}
