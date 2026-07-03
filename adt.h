/*********************************************************************
 * 文件名：adt.h
 * 作者：  郭佳鑫 
 * 功能简述：
 *   定义选课管理系统的核心数据结构、常量和接口声明。
 *   提供 EnrollmentRecord、DataStructure、排序上下文等基础类型。
 *********************************************************************/

#ifndef ADT_H
#define ADT_H

// ===================== 常量 =====================
#define STUDENT_ID_LEN 12
#define NAME_LEN 30
#define COLLEGE_LEN 50
#define COURSE_ID_LEN 8
#define COURSE_NAME_LEN 50
#define SEMESTER_LEN 7

#define MAX_SORT_KEYS 5   /* 多关键字排序最多支持的字段数 */

// ===================== 选课记录结构体 =====================
typedef struct {
    char student_id[STUDENT_ID_LEN + 1];
    char name[NAME_LEN + 1];
    char college[COLLEGE_LEN + 1];
    char course_id[COURSE_ID_LEN + 1];
    char course_name[COURSE_NAME_LEN + 1];
    float credit;
    char semester[SEMESTER_LEN + 1];
    int year, month, day;
    int score;
} EnrollmentRecord;

// ===================== 操作函数指针类型 =====================
typedef void (*InsertFunc)(void *storage, EnrollmentRecord rec);
typedef void (*ModifyScoreFunc)(void *storage, const char *sid, const char *cid, int score);
typedef EnrollmentRecord* (*SearchFunc)(void *storage, const char *sid, const char *cid, int *count);
typedef int (*DeleteFunc)(void *storage, const char *sid, const char *cid);
typedef void (*ListAllFunc)(void *storage);
typedef void (*GetAllFunc)(void *storage, EnrollmentRecord **arr, int *size);
typedef void (*CleanExpiredFunc)(void *storage, int before_year);
typedef void (*DestroyFunc)(void *storage);

// ===================== 数据结构封装 =====================
typedef struct {
    int type;                /* 0-链表，1-哈希表 */
    void *storage;           /* 实际存储结构指针（LinkedList* 或 HashTable*） */
    InsertFunc insert;
    ModifyScoreFunc modify_score;
    SearchFunc search;
    DeleteFunc delete;
    ListAllFunc list_all;
    GetAllFunc get_all_records;
    CleanExpiredFunc clean_expired;
    DestroyFunc destroy;
} DataStructure;

// ===================== 排序上下文（全局） =====================
typedef struct {
    int sort_key;    /* 0-学号,1-姓名,2-课程名,3-成绩,4-日期 */
    int ascending;   /* 1升序,0降序 */
} SortContext;

typedef struct {
    int sort_keys[MAX_SORT_KEYS];   /* 字段编号数组 */
    int sort_dirs[MAX_SORT_KEYS];   /* 方向  1升序,0降序 */
    int count;                      /* 实际使用的字段数 */
} MultiSortContext;

extern SortContext sort_ctx;           /* 单关键字排序全局配置 */
extern MultiSortContext multi_sort_ctx; /* 多关键字排序全局配置 */

// ===================== 通用接口声明 =====================

/**
 * 初始化数据存储结构
 * @param ds   未初始化的 DataStructure 指针
 * @param type 0-链表，1-哈希表
 */
void ds_init(DataStructure *ds, int type);

/**
 * 转换数据存储结构（保留原有记录）
 * @param ds       当前存储结构指针
 * @param new_type 目标存储类型
 */
void ds_convert(DataStructure *ds, int new_type);

/**
 * 销毁数据存储结构，释放全部资源
 * @param ds 已初始化的 DataStructure 指针
 */
void ds_destroy(DataStructure *ds);

#endif /* ADT_H */
