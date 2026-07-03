/*********************************************************************
 * 文件名：util.c
 * 说明：  工具函数实现，包括：文件读写、数据生成、菜单交互、
 *         统计分析、综合性能测试与复杂度验证。
 *********************************************************************/

#include "util.h"
#include "list.h"
#include "hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>          // 高精度计时
#include <math.h>             // log10

// ==================== 全局排序参数 ====================
SortContext sort_ctx;
MultiSortContext multi_sort_ctx;

// ==================== 辅助：清空输入缓冲区 ====================
void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// ==================== 数据结构初始化/转换/销毁 ====================
void ds_init(DataStructure *ds, int type) {
    if (type == 0) {
        list_attach(ds);
        ds->type = 0;
    } else {
        hash_attach(ds);
        ds->type = 1;
    }
}

void ds_convert(DataStructure *ds, int new_type) {
    if (ds->type == new_type) return;
    int count = 0;
    EnrollmentRecord *records = NULL;
    ds->get_all_records(ds->storage, &records, &count);
    ds->destroy(ds->storage);
    free(ds->storage);
    ds_init(ds, new_type);
    for (int i = 0; i < count; i++) {
        ds->insert(ds->storage, records[i]);
    }
    free(records);
}

void ds_destroy(DataStructure *ds) {
    if (ds->destroy) ds->destroy(ds->storage);
    free(ds->storage);
}

// ==================== CSV 文件读写 ====================
int load_from_csv(const char *filename, EnrollmentRecord **data, int *count) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;
    char line[512];
    int cap = 10;
    *data = (EnrollmentRecord*)malloc(sizeof(EnrollmentRecord) * cap);
    *count = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (*count >= cap) {
            cap *= 2;
            *data = (EnrollmentRecord*)realloc(*data, sizeof(EnrollmentRecord) * cap);
        }
        EnrollmentRecord rec;
        if (sscanf(line, "%12[^,],%30[^,],%50[^,],%8[^,],%50[^,],%f,%7[^,],%d-%d-%d,%d",
                   rec.student_id, rec.name, rec.college,
                   rec.course_id, rec.course_name,
                   &rec.credit, rec.semester,
                   &rec.year, &rec.month, &rec.day, &rec.score) == 11) {
            (*data)[(*count)++] = rec;
        }
    }
    fclose(fp);
    return 1;
}

void save_to_csv(const char *filename, const EnrollmentRecord *data, int count) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return;
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%s,%s,%s,%s,%s,%.1f,%s,%d-%02d-%02d,%d\n",
                data[i].student_id, data[i].name, data[i].college,
                data[i].course_id, data[i].course_name,
                data[i].credit, data[i].semester,
                data[i].year, data[i].month, data[i].day,
                data[i].score);
    }
    fclose(fp);
}

// ==================== 随机数据生成 ====================
static const char *surnames[] = {"赵","钱","孙","李","周","吴","郑","王","冯","陈"};
#define SURNAMES_COUNT (sizeof(surnames)/sizeof(surnames[0]))
static const char *given_names[] = {"伟","芳","敏","强","丽","磊","勇","静","小华","国安"};
#define GIVEN_NAMES_COUNT (sizeof(given_names)/sizeof(given_names[0]))
static const char *colleges[] = {"计算机科学与工程学院","数学学院","人工智能学院","数据科学学院"};
#define COLLEGES_COUNT (sizeof(colleges)/sizeof(colleges[0]))
static const char *course_names[] = {"数据结构与算法","操作系统","计算机网络","数据库原理","软件工程","编译原理","人工智能","机器学习"};
#define COURSE_NAMES_COUNT (sizeof(course_names)/sizeof(course_names[0]))

void generate_records(EnrollmentRecord *records, int count) {
    for (int i = 0; i < count; i++) {
        snprintf(records[i].student_id, sizeof(records[i].student_id), "2024%04d", rand() % 10000);
        snprintf(records[i].name, sizeof(records[i].name), "%s%s",
                 surnames[rand() % SURNAMES_COUNT],
                 given_names[rand() % GIVEN_NAMES_COUNT]);
        strncpy(records[i].college, colleges[rand() % COLLEGES_COUNT], COLLEGE_LEN);
        records[i].college[COLLEGE_LEN] = '\0';

        int course_idx = rand() % COURSE_NAMES_COUNT;
        strncpy(records[i].course_name, course_names[course_idx], COURSE_NAME_LEN);
        records[i].course_name[COURSE_NAME_LEN] = '\0';
        snprintf(records[i].course_id, sizeof(records[i].course_id), "CS%04d", 1001 + course_idx);

        records[i].credit = 2.0f + (float)(rand() % 31) / 10.0f;
        records[i].year = 2020 + rand() % 5;
        records[i].month = 1 + rand() % 12;
        records[i].day = 1 + rand() % 28;
        int sem = (records[i].month <= 6) ? 1 : 2;
        snprintf(records[i].semester, sizeof(records[i].semester), "%d-%02d", records[i].year, sem);
        records[i].score = rand() % 101;
    }
}

// ==================== 比较函数（用于 qsort） ====================
static int compare_record(const void *a, const void *b) {
    const EnrollmentRecord *ra = (const EnrollmentRecord*)a;
    const EnrollmentRecord *rb = (const EnrollmentRecord*)b;

    if (multi_sort_ctx.count > 0) {
        for (int i = 0; i < multi_sort_ctx.count; i++) {
            int key = multi_sort_ctx.sort_keys[i];
            int dir = multi_sort_ctx.sort_dirs[i];
            int result = 0;
            switch (key) {
                case 0: result = strcmp(ra->student_id, rb->student_id); break;
                case 1: result = strcmp(ra->name, rb->name); break;
                case 2: result = strcmp(ra->course_name, rb->course_name); break;
                case 3:
                    if (ra->score < rb->score) result = -1;
                    else if (ra->score > rb->score) result = 1;
                    break;
                case 4:
                    if (ra->year != rb->year) result = ra->year - rb->year;
                    else if (ra->month != rb->month) result = ra->month - rb->month;
                    else result = ra->day - rb->day;
                    break;
            }
            if (result != 0) return dir ? result : -result;
        }
        return 0;
    }

    int result = 0;
    switch (sort_ctx.sort_key) {
        case 0: result = strcmp(ra->student_id, rb->student_id); break;
        case 1: result = strcmp(ra->name, rb->name); break;
        case 2: result = strcmp(ra->course_name, rb->course_name); break;
        case 3:
            if (ra->score < rb->score) result = -1;
            else if (ra->score > rb->score) result = 1;
            break;
        case 4:
            if (ra->year != rb->year) result = ra->year - rb->year;
            else if (ra->month != rb->month) result = ra->month - rb->month;
            else result = ra->day - rb->day;
            break;
    }
    return sort_ctx.ascending ? result : -result;
}

// ==================== 交互式功能 ====================
void add_record(DataStructure *ds) {
    EnrollmentRecord rec;
    printf("格式: 学号 姓名 学院 课程编号 课程名 学分 选课学期 选课日期(yyyy-mm-dd) 成绩\n");
    if (scanf("%12s %30s %50s %8s %50s %f %7s %d-%d-%d %d",
              rec.student_id, rec.name, rec.college,
              rec.course_id, rec.course_name,
              &rec.credit, rec.semester,
              &rec.year, &rec.month, &rec.day, &rec.score) != 11) {
        clear_input_buffer();
        printf("输入格式错误。\n");
        return;
    }
    ds->insert(ds->storage, rec);
    printf("记录已添加。\n");
}

void modify_score(DataStructure *ds) {
    char sid[STUDENT_ID_LEN+1], cid[COURSE_ID_LEN+1];
    int score;
    printf("学号 课程编号 新成绩: ");
    if (scanf("%12s %8s %d", sid, cid, &score) != 3) {
        clear_input_buffer();
        printf("格式错误。\n");
        return;
    }
    ds->modify_score(ds->storage, sid, cid, score);
    printf("成绩已更新。\n");
}

void search_record(DataStructure *ds) {
    char sid[STUDENT_ID_LEN+1], cid[COURSE_ID_LEN+1];
    printf("学号 课程编号: ");
    if (scanf("%12s %8s", sid, cid) != 2) {
        clear_input_buffer();
        printf("格式错误。\n");
        return;
    }
    int count;
    EnrollmentRecord *res = ds->search(ds->storage, sid, cid, &count);
    if (res) {
        printf("%-12s %-8s %-25s %-8s %-25s %4s %7s %10s %4s\n",
               "学号", "姓名", "学院", "课程编号", "课程名", "学分", "学期", "日期", "成绩");
        for (int i = 0; i < count; i++) {
            printf("%-12s %-8s %-25s %-8s %-25s %4.1f %7s %4d-%02d-%02d %4d\n",
                   res[i].student_id, res[i].name, res[i].college,
                   res[i].course_id, res[i].course_name,
                   res[i].credit, res[i].semester,
                   res[i].year, res[i].month, res[i].day,
                   res[i].score);
        }
        free(res);
    } else {
        printf("未找到记录。\n");
    }
}

void delete_record(DataStructure *ds) {
    char sid[STUDENT_ID_LEN+1], cid[COURSE_ID_LEN+1];
    printf("学号 课程编号: ");
    if (scanf("%12s %8s", sid, cid) != 2) {
        clear_input_buffer();
        printf("格式错误。\n");
        return;
    }
    if (ds->delete(ds->storage, sid, cid))
        printf("删除成功。\n");
    else
        printf("未找到记录。\n");
}

void list_all_records(DataStructure *ds) {
    ds->list_all(ds->storage);
}

void filter_and_sort(DataStructure *ds) {
    EnrollmentRecord *arr = NULL;
    int size = 0;
    ds->get_all_records(ds->storage, &arr, &size);
    if (size == 0) {
        printf("没有数据。\n");
        return;
    }

    int filter_choice = 0;
    printf("是否需要筛选数据？(1-是, 0-否): ");
    if (scanf("%d", &filter_choice) != 1) {
        clear_input_buffer();
        filter_choice = 0;
    }
    clear_input_buffer();

    EnrollmentRecord *filtered = arr;
    int filtered_size = size;

    if (filter_choice) {
        char course_name[COURSE_NAME_LEN+1] = "";
        int match_type = 0;
        char semester[SEMESTER_LEN+1] = "";
        int score_min = -1, score_max = -1;
        char college[COLLEGE_LEN+1] = "";

        printf("课程名称（空表示忽略）: ");
        fgets(course_name, COURSE_NAME_LEN+1, stdin);
        course_name[strcspn(course_name, "\n")] = 0;
        if (strlen(course_name) > 0) {
            printf("匹配方式 (1-精确, 2-模糊): ");
            if (scanf("%d", &match_type) != 1 || (match_type != 1 && match_type != 2)) {
                clear_input_buffer();
                match_type = 0;
            }
            clear_input_buffer();
        }

        printf("选课学期（例如2024-01，空忽略）: ");
        fgets(semester, SEMESTER_LEN+1, stdin);
        semester[strcspn(semester, "\n")] = 0;

        printf("成绩区间（最低分 最高分，-1 表示不限制）: ");
        char line_buf[64];
        if (fgets(line_buf, sizeof(line_buf), stdin)) {
            line_buf[strcspn(line_buf, "\n")] = 0;
            int parsed = sscanf(line_buf, "%d %d", &score_min, &score_max);
            if (parsed == 0) score_min = score_max = -1;
            else if (parsed == 1) score_max = score_min;
        }

        printf("学院（空忽略）: ");
        fgets(college, COLLEGE_LEN+1, stdin);
        college[strcspn(college, "\n")] = 0;

        filtered = (EnrollmentRecord*)malloc(sizeof(EnrollmentRecord) * size);
        filtered_size = 0;
        for (int i = 0; i < size; i++) {
            EnrollmentRecord *r = &arr[i];
            if (match_type != 0 && strlen(course_name) > 0) {
                if (match_type == 1) {
                    if (strcmp(r->course_name, course_name) != 0) continue;
                } else {
                    if (strstr(r->course_name, course_name) == NULL) continue;
                }
            }
            if (strlen(semester) > 0 && strcmp(r->semester, semester) != 0) continue;
            if (score_min != -1 && r->score < score_min) continue;
            if (score_max != -1 && r->score > score_max) continue;
            if (strlen(college) > 0 && strcmp(r->college, college) != 0) continue;
            filtered[filtered_size++] = *r;
        }
        free(arr);
        if (filtered_size == 0) {
            printf("没有符合条件的数据。\n");
            free(filtered);
            return;
        }
    }

    printf("筛选结果: %d 条\n", filtered_size);

    int sort_choice = 0;
    printf("是否需要排序(1-是, 0-否): ");
    if (scanf("%d", &sort_choice) != 1) {
        clear_input_buffer();
        sort_choice = 0;
    }
    clear_input_buffer();
    if (sort_choice) {
        multi_sort_ctx.count = 0;
        printf("请输入排序字段数量（最多%d，-1 取消）: ", MAX_SORT_KEYS);
        int count;
        if (scanf("%d", &count) != 1) {
            clear_input_buffer();
            count = 0;
        }
        clear_input_buffer();
        if (count > MAX_SORT_KEYS) count = MAX_SORT_KEYS;
        if (count < 0) count = 0;
        for (int i = 0; i < count; i++) {
            printf("字段%d (0-学号,1-姓名,2-课程名,3-成绩,4-日期): ", i+1);
            int key;
            if (scanf("%d", &key) != 1 || key < 0 || key > 4) {
                clear_input_buffer();
                key = 0;
            }
            printf("方向 (1-升序, 0-降序): ");
            int dir;
            if (scanf("%d", &dir) != 1 || (dir != 0 && dir != 1)) {
                clear_input_buffer();
                dir = 1;
            }
            multi_sort_ctx.sort_keys[i] = key;
            multi_sort_ctx.sort_dirs[i] = dir;
            multi_sort_ctx.count++;
            clear_input_buffer();
        }
        if (multi_sort_ctx.count > 0) {
            qsort(filtered, filtered_size, sizeof(EnrollmentRecord), compare_record);
        }
    }

    int show = filtered_size < 20 ? filtered_size : 20;
    printf("%-12s %-8s %-25s %-8s %-25s %4s %7s %10s %4s\n",
           "学号", "姓名", "学院", "课程编号", "课程名", "学分", "学期", "日期", "成绩");
    for (int i = 0; i < show; i++) {
        printf("%-12s %-8s %-25s %-8s %-25s %4.1f %7s %4d-%02d-%02d %4d\n",
               filtered[i].student_id, filtered[i].name, filtered[i].college,
               filtered[i].course_id, filtered[i].course_name,
               filtered[i].credit, filtered[i].semester,
               filtered[i].year, filtered[i].month, filtered[i].day,
               filtered[i].score);
    }
    if (filtered_size > 20) printf("... (仅显示前20条)\n");

    int export_choice = 0;
    printf("是否导出到文件？(1-是, 0-否): ");
    if (scanf("%d", &export_choice) != 1) {
        clear_input_buffer();
        export_choice = 0;
    }
    clear_input_buffer();
    if (export_choice) {
        char filename[256];
        printf("请输入文件名（如 result.csv）: ");
        fgets(filename, sizeof(filename), stdin);
        filename[strcspn(filename, "\n")] = 0;
        save_to_csv(filename, filtered, filtered_size);
        printf("已保存到 %s\n", filename);
    }

    free(filtered);
}

void clean_expired(DataStructure *ds) {
    int before_year;
    printf("请输入年份，删除该年份之前的记录: ");
    if (scanf("%d", &before_year) != 1) {
        clear_input_buffer();
        return;
    }
    ds->clean_expired(ds->storage, before_year);
    printf("清理完成。\n");
}

void batch_generate(DataStructure *ds) {
    int clear;
    printf("是否清空现有数据再生成？(1-是 0-否): ");
    if (scanf("%d", &clear) != 1) {
        clear_input_buffer();
        clear = 0;
    }
    int count;
    printf("请输入需要生成的记录数: ");
    if (scanf("%d", &count) != 1 || count <= 0) {
        clear_input_buffer();
        printf("无效数量。\n");
        return;
    }
    if (clear) {
        ds_destroy(ds);
        ds_init(ds, ds->type);
    }
    EnrollmentRecord *recs = (EnrollmentRecord*)malloc(sizeof(EnrollmentRecord) * count);
    generate_records(recs, count);
    for (int i = 0; i < count; i++) {
        ds->insert(ds->storage, recs[i]);
    }
    printf("成功生成并添加 %d 条记录。\n", count);
    free(recs);
}

// ==================== 统计分析 ====================
void course_enrollment_stats(DataStructure *ds) {
    EnrollmentRecord *arr = NULL; int size = 0;
    ds->get_all_records(ds->storage, &arr, &size);
    if (size == 0) { printf("无选课记录。\n"); return; }

    int default_cap = 50;
    printf("请输入所有课程的默认容量（默认50）: ");
    if (scanf("%d", &default_cap) != 1) { clear_input_buffer(); default_cap = 50; }
    if (default_cap <= 0) default_cap = 50;

    typedef struct { char course_id[COURSE_ID_LEN+1], course_name[COURSE_NAME_LEN+1]; int count; } CourseStat;
    CourseStat *stats = (CourseStat*)malloc(sizeof(CourseStat) * size);
    int stat_cnt = 0;
    for (int i = 0; i < size; i++) {
        int found = -1;
        for (int j = 0; j < stat_cnt; j++) if (strcmp(stats[j].course_id, arr[i].course_id) == 0) { found = j; break; }
        if (found >= 0) stats[found].count++;
        else {
            strcpy(stats[stat_cnt].course_id, arr[i].course_id);
            strcpy(stats[stat_cnt].course_name, arr[i].course_name);
            stats[stat_cnt].count = 1; stat_cnt++;
        }
    }
    printf("\n%-10s %-30s %6s %10s\n", "课程编号", "课程名称", "选课人数", "容量使用率");
    printf("---------------------------------------------------------------\n");
    for (int i = 0; i < stat_cnt; i++) {
        float usage = (float)stats[i].count / default_cap * 100;
        printf("%-10s %-30s %6d %9.1f%%\n", stats[i].course_id, stats[i].course_name, stats[i].count, usage);
    }
    free(stats); free(arr);
}

void student_credits_stats(DataStructure *ds) {
    EnrollmentRecord *arr = NULL; int size = 0;
    ds->get_all_records(ds->storage, &arr, &size);
    if (size == 0) { printf("无选课记录。\n"); return; }
    typedef struct { char student_id[STUDENT_ID_LEN+1], name[NAME_LEN+1]; int course_count; float total_credit; } StudentStat;
    StudentStat *stats = (StudentStat*)malloc(sizeof(StudentStat) * size);
    int stat_cnt = 0;
    for (int i = 0; i < size; i++) {
        int found = -1;
        for (int j = 0; j < stat_cnt; j++) if (strcmp(stats[j].student_id, arr[i].student_id) == 0) { found = j; break; }
        if (found >= 0) { stats[found].course_count++; stats[found].total_credit += arr[i].credit; }
        else { strcpy(stats[stat_cnt].student_id, arr[i].student_id); strcpy(stats[stat_cnt].name, arr[i].name); stats[stat_cnt].course_count = 1; stats[stat_cnt].total_credit = arr[i].credit; stat_cnt++; }
    }
    printf("\n%-12s %-10s %6s %8s\n", "学号", "姓名", "选课门数", "总学分");
    printf("------------------------------------------\n");
    for (int i = 0; i < stat_cnt; i++) printf("%-12s %-10s %6d %6.1f\n", stats[i].student_id, stats[i].name, stats[i].course_count, stats[i].total_credit);
    free(stats); free(arr);
}

void college_distribution_stats(DataStructure *ds) {
    EnrollmentRecord *arr = NULL; int size = 0;
    ds->get_all_records(ds->storage, &arr, &size);
    if (size == 0) { printf("无选课记录。\n"); return; }
    typedef struct { char college[COLLEGE_LEN+1]; int count; } CollegeStat;
    CollegeStat *stats = (CollegeStat*)malloc(sizeof(CollegeStat) * size);
    int stat_cnt = 0;
    for (int i = 0; i < size; i++) {
        int found = -1;
        for (int j = 0; j < stat_cnt; j++) if (strcmp(stats[j].college, arr[i].college) == 0) { found = j; break; }
        if (found >= 0) stats[found].count++;
        else { strcpy(stats[stat_cnt].college, arr[i].college); stats[stat_cnt].count = 1; stat_cnt++; }
    }
    for (int i = 0; i < stat_cnt - 1; i++) for (int j = i+1; j < stat_cnt; j++) if (stats[j].count > stats[i].count) { CollegeStat t = stats[i]; stats[i] = stats[j]; stats[j] = t; }
    printf("\n%-30s %8s %10s\n", "学院", "选课人次", "占比");
    printf("--------------------------------------------------\n");
    for (int i = 0; i < stat_cnt; i++) printf("%-30s %8d %9.1f%%\n", stats[i].college, stats[i].count, (float)stats[i].count/size*100);
    free(stats); free(arr);
}

void semester_stats(DataStructure *ds) {
    EnrollmentRecord *arr = NULL; int size = 0;
    ds->get_all_records(ds->storage, &arr, &size);
    if (size == 0) { printf("无选课记录。\n"); return; }
    char **semesters = (char**)malloc(sizeof(char*) * size);
    int sem_cnt = 0;
    for (int i = 0; i < size; i++) {
        int found = 0;
        for (int j = 0; j < sem_cnt; j++) if (strcmp(semesters[j], arr[i].semester) == 0) { found = 1; break; }
        if (!found) semesters[sem_cnt++] = strdup(arr[i].semester);
    }
    for (int i = 0; i < sem_cnt - 1; i++) for (int j = i+1; j < sem_cnt; j++) if (strcmp(semesters[i], semesters[j]) > 0) { char *t = semesters[i]; semesters[i] = semesters[j]; semesters[j] = t; }
    printf("\n%-10s %10s %10s %10s\n", "学期", "选课人次", "课程数", "学生数");
    printf("---------------------------------------------\n");
    for (int s = 0; s < sem_cnt; s++) {
        int rec_cnt = 0;
        for (int i = 0; i < size; i++) if (strcmp(arr[i].semester, semesters[s]) == 0) rec_cnt++;
        char **stu_ids = (char**)malloc(sizeof(char*) * rec_cnt), **crs_ids = (char**)malloc(sizeof(char*) * rec_cnt);
        int stu_cnt = 0, crs_cnt = 0;
        for (int i = 0; i < size; i++) {
            if (strcmp(arr[i].semester, semesters[s]) != 0) continue;
            int found_stu = 0;
            for (int j = 0; j < stu_cnt; j++) if (strcmp(stu_ids[j], arr[i].student_id) == 0) { found_stu = 1; break; }
            if (!found_stu) stu_ids[stu_cnt++] = strdup(arr[i].student_id);
            int found_crs = 0;
            for (int j = 0; j < crs_cnt; j++) if (strcmp(crs_ids[j], arr[i].course_id) == 0) { found_crs = 1; break; }
            if (!found_crs) crs_ids[crs_cnt++] = strdup(arr[i].course_id);
        }
        printf("%-10s %10d %10d %10d\n", semesters[s], rec_cnt, crs_cnt, stu_cnt);
        for (int i = 0; i < stu_cnt; i++) free(stu_ids[i]);
        for (int i = 0; i < crs_cnt; i++) free(crs_ids[i]);
        free(stu_ids); free(crs_ids);
    }
    for (int i = 0; i < sem_cnt; i++) free(semesters[i]);
    free(semesters); free(arr);
}

void score_distribution_stats(DataStructure *ds) {
    EnrollmentRecord *arr = NULL; int size = 0;
    ds->get_all_records(ds->storage, &arr, &size);
    if (size == 0) { printf("无选课记录。\n"); return; }
    int exc = 0, good = 0, med = 0, pass = 0, fail = 0;
    for (int i = 0; i < size; i++) {
        int s = arr[i].score;
        if (s >= 90) exc++;
        else if (s >= 80) good++;
        else if (s >= 70) med++;
        else if (s >= 60) pass++;
        else fail++;
    }
    printf("\n成绩分布统计 (总记录 %d 条):\n", size);
    printf("优秀 (90-100): %d (占比 %.1f%%)\n", exc, (float)exc/size*100);
    printf("良好 (80-89) : %d (占比 %.1f%%)\n", good, (float)good/size*100);
    printf("中等 (70-79) : %d (占比 %.1f%%)\n", med, (float)med/size*100);
    printf("及格 (60-69) : %d (占比 %.1f%%)\n", pass, (float)pass/size*100);
    printf("不及格 (<60) : %d (占比 %.1f%%)\n", fail, (float)fail/size*100);
    free(arr);
}

void statistics_menu(DataStructure *ds) {
    int choice;
    while (1) {
        printf("\n====== 统计分析子菜单 ======\n");
        printf("0. 基础统计\n1. 每门课程选课人数及容量使用率\n2. 每位学生选课门数及总学分\n");
        printf("3. 按学院选课人次分布\n4. 按学期统计\n5. 成绩分布统计\n9. 返回\n");
        printf("请输入选择: ");
        if (scanf("%d", &choice) != 1) { clear_input_buffer(); continue; }
        clear_input_buffer();
        switch (choice) {
            case 0: {
                EnrollmentRecord *arr = NULL; int size = 0;
                ds->get_all_records(ds->storage, &arr, &size);
                if (size == 0) { printf("无数据。\n"); break; }
                int sum = 0, max = -1, min = 101; float cred = 0;
                for (int i = 0; i < size; i++) {
                    int s = arr[i].score; sum += s;
                    if (s > max) max = s; if (s < min) min = s;
                    cred += arr[i].credit;
                }
                printf("总记录数: %d\n平均分: %.2f\n最高分: %d\n最低分: %d\n总学分: %.1f\n", size, (float)sum/size, max, min, cred);
                free(arr); break;
            }
            case 1: course_enrollment_stats(ds); break;
            case 2: student_credits_stats(ds); break;
            case 3: college_distribution_stats(ds); break;
            case 4: semester_stats(ds); break;
            case 5: score_distribution_stats(ds); break;
            case 9: return;
            default: printf("无效选择。\n");
        }
    }
}

// ==================== 高精度计时 & 内存估算 ====================
static double get_highres_time_ms(void) {
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0) {
        QueryPerformanceFrequency(&freq);
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000.0 / freq.QuadPart;
}

static double estimate_memory(int type, int n) {
    size_t record_size = sizeof(EnrollmentRecord);
    size_t ptr_size = sizeof(void *);
    double bytes;

    if (type == 0) {  // 链表：节点 = 记录 + prev + next 指针
        bytes = (double)(n * (record_size + 2 * ptr_size));
    } else {          // 哈希表：节点 = 记录 + next；桶数组 2n+1 个指针
        int bucket_count = n * 2 + 1;
        bytes = (double)(n * (record_size + ptr_size) + bucket_count * ptr_size);
    }
    return bytes / (1024.0 * 1024.0);
}

// ==================== 综合性能测试 & 复杂度验证 ====================
#define TEST_ROUNDS 3
#define OP_REPEAT   500

void comprehensive_performance_test(void) {
    int sizes[] = {100, 1000, 10000};
    const char *type_names[] = {"链表", "哈希表"};
    const char *op_names[] = {"插入", "查找x500", "修改x500", "删除x500", "遍历", "排序"};

    const char *theory[2][6] = {
        {"O(1)", "O(n)", "O(n)", "O(n)", "O(n)", "O(n log n)"},
        {"O(1)", "O(1)", "O(1)", "O(1)", "O(n)", "O(n log n)"}
    };

    double times[3][2][6];          // [规模][结构][操作]
    double memory_usage[3][2];      // [规模][结构] (MB)

    srand(0);
    int max_n = 10000;
    EnrollmentRecord *all_records = (EnrollmentRecord*)malloc(sizeof(EnrollmentRecord) * max_n);
    generate_records(all_records, max_n);

    printf("\n================================================================================\n");
    printf("%-6s %-6s %-8s %-10s %-10s %-12s\n",
           "规模", "结构", "操作", "理论", "耗时(ms)", "内存(MB)");
    printf("--------------------------------------------------------------------------------\n");

    for (int s = 0; s < 3; s++) {
        int n = sizes[s];
        for (int t = 0; t < 2; t++) {
            int type = t;

            // ---- 1. 插入 ----
            double insert_avg = 0.0;
            for (int r = 0; r < TEST_ROUNDS; r++) {
                DataStructure ds;
                ds_init(&ds, type);
                double t0 = get_highres_time_ms();
                for (int i = 0; i < n; i++) ds.insert(ds.storage, all_records[i]);
                double t1 = get_highres_time_ms();
                insert_avg += (t1 - t0);
                ds_destroy(&ds);
            }
            insert_avg /= TEST_ROUNDS;
            times[s][t][0] = insert_avg;

            double mem_used = estimate_memory(type, n);
            memory_usage[s][t] = mem_used;

            printf("%-6d %-6s %-8s %-10s %-10.3f %-12.4f\n",
                   n, type_names[t], op_names[0], theory[t][0], insert_avg, mem_used);

            // ---- 2. 查找 ----
            double search_avg = 0.0;
            for (int r = 0; r < TEST_ROUNDS; r++) {
                DataStructure ds;
                ds_init(&ds, type);
                for (int i = 0; i < n; i++) ds.insert(ds.storage, all_records[i]);
                double t0 = get_highres_time_ms();
                for (int k = 0; k < OP_REPEAT; k++) {
                    int idx = rand() % n;
                    char sid[STUDENT_ID_LEN+1], cid[COURSE_ID_LEN+1];
                    strcpy(sid, all_records[idx].student_id);
                    strcpy(cid, all_records[idx].course_id);
                    int cnt = 0;
                    EnrollmentRecord *res = ds.search(ds.storage, sid, cid, &cnt);
                    if (res) free(res);
                }
                double t1 = get_highres_time_ms();
                search_avg += (t1 - t0);
                ds_destroy(&ds);
            }
            search_avg /= TEST_ROUNDS;
            times[s][t][1] = search_avg;
            printf("%-6s %-6s %-8s %-10s %-10.3f %-12s\n",
                   "", "", op_names[1], theory[t][1], search_avg, "-");

            // ---- 3. 修改 ----
            double modify_avg = 0.0;
            for (int r = 0; r < TEST_ROUNDS; r++) {
                DataStructure ds;
                ds_init(&ds, type);
                for (int i = 0; i < n; i++) ds.insert(ds.storage, all_records[i]);
                double t0 = get_highres_time_ms();
                for (int k = 0; k < OP_REPEAT; k++) {
                    int idx = rand() % n;
                    ds.modify_score(ds.storage, all_records[idx].student_id,
                                    all_records[idx].course_id, 90);
                }
                double t1 = get_highres_time_ms();
                modify_avg += (t1 - t0);
                ds_destroy(&ds);
            }
            modify_avg /= TEST_ROUNDS;
            times[s][t][2] = modify_avg;
            printf("%-6s %-6s %-8s %-10s %-10.3f %-12s\n",
                   "", "", op_names[2], theory[t][2], modify_avg, "-");

            // ---- 4. 删除 ----
            double delete_avg = 0.0;
            for (int r = 0; r < TEST_ROUNDS; r++) {
                DataStructure ds;
                ds_init(&ds, type);
                for (int i = 0; i < n; i++) ds.insert(ds.storage, all_records[i]);
                int del_count = (n < OP_REPEAT) ? n : OP_REPEAT;
                int *del_idx = (int*)malloc(sizeof(int) * del_count);
                for (int i = 0; i < del_count; i++) del_idx[i] = rand() % n;
                double t0 = get_highres_time_ms();
                for (int k = 0; k < del_count; k++) {
                    ds.delete(ds.storage, all_records[del_idx[k]].student_id,
                              all_records[del_idx[k]].course_id);
                }
                double t1 = get_highres_time_ms();
                delete_avg += (t1 - t0);
                free(del_idx);
                ds_destroy(&ds);
            }
            delete_avg /= TEST_ROUNDS;
            times[s][t][3] = delete_avg;
            printf("%-6s %-6s %-8s %-10s %-10.3f %-12s\n",
                   "", "", op_names[3], theory[t][3], delete_avg, "-");

            // ---- 5. 遍历 ----
            double traverse_avg = 0.0;
            for (int r = 0; r < TEST_ROUNDS; r++) {
                DataStructure ds;
                ds_init(&ds, type);
                for (int i = 0; i < n; i++) ds.insert(ds.storage, all_records[i]);
                double t0 = get_highres_time_ms();
                EnrollmentRecord *arr = NULL; int cnt = 0;
                ds.get_all_records(ds.storage, &arr, &cnt);
                double t1 = get_highres_time_ms();
                if (arr) free(arr);
                traverse_avg += (t1 - t0);
                ds_destroy(&ds);
            }
            traverse_avg /= TEST_ROUNDS;
            times[s][t][4] = traverse_avg;
            printf("%-6s %-6s %-8s %-10s %-10.3f %-12s\n",
                   "", "", op_names[4], theory[t][4], traverse_avg, "-");

            // ---- 6. 排序 ----
            double sort_avg = 0.0;
            for (int r = 0; r < TEST_ROUNDS; r++) {
                DataStructure ds;
                ds_init(&ds, type);
                for (int i = 0; i < n; i++) ds.insert(ds.storage, all_records[i]);
                EnrollmentRecord *arr = NULL; int cnt = 0;
                ds.get_all_records(ds.storage, &arr, &cnt);
                sort_ctx.sort_key = 0;
                sort_ctx.ascending = 1;
                multi_sort_ctx.count = 0;
                double t0 = get_highres_time_ms();
                qsort(arr, cnt, sizeof(EnrollmentRecord), compare_record);
                double t1 = get_highres_time_ms();
                sort_avg += (t1 - t0);
                free(arr);
                ds_destroy(&ds);
            }
            sort_avg /= TEST_ROUNDS;
            times[s][t][5] = sort_avg;
            printf("%-6s %-6s %-8s %-10s %-10.3f %-12s\n",
                   "", "", op_names[5], theory[t][5], sort_avg, "-");

            printf("--------------------------------------------------------------------------------\n");
        }
    }
    free(all_records);

    // ==================== 复杂度验证分析报告（修订版） ====================
    printf("\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║              复杂度验证分析报告                              ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    printf("【测试环境】\n");
    printf("CPU 架构: %u 位, 核心数: %u\n",
           sysInfo.wProcessorArchitecture == 9 ? 64 : 32,
           sysInfo.dwNumberOfProcessors);
    printf("测试轮次: %d 轮 / 操作, 查找/修改/删除内部重复：%d 次\n\n",
           TEST_ROUNDS, OP_REPEAT);

    for (int op = 0; op < 6; op++) {
        printf("【%s】\n", op_names[op]);
        printf("链表理论: %s, 哈希表理论: %s\n", theory[0][op], theory[1][op]);

        double list_1000 = times[1][0][op];
        double list_10000 = times[2][0][op];
        double hash_1000 = times[1][1][op];
        double hash_10000 = times[2][1][op];

        if (op == 0) { // 插入操作 – 总时间与 n 成正比
            printf("（注：此处耗时是插入全部 n 条的总时间，理论应随规模线性增长约 10 倍）\n");
            if (list_1000 > 0.001) {
                double ratio = list_10000 / list_1000;
                printf("链表总时间：1000条 %.3f ms, 10000条 %.3f ms, 增长 %.2f 倍\n", list_1000, list_10000, ratio);
                if (ratio > 6 && ratio < 15) printf("   → 增长基本符合 O(n) 总时间（单次插入 O(1)）。\n");
                else printf("   → 偏差较大，可能受内存分配等因素影响。\n");
            }
            if (hash_1000 > 0.001) {
                double ratio = hash_10000 / hash_1000;
                printf("哈希表总时间：1000条 %.3f ms, 10000条 %.3f ms, 增长 %.2f 倍\n", hash_1000, hash_10000, ratio);
                if (ratio > 6 && ratio < 15) printf("   → 增长基本符合 O(n) 总时间（单次插入 O(1)，扩容分摊）。\n");
                else printf("   → 偏差较大，请检查哈希函数或扩容策略。\n");
            }
        } else if (op == 1 || op == 2 || op == 3) { // 查找/修改/删除：500次操作总时间
            printf("（注：此处耗时是执行 %d 次操作的总时间）\n", OP_REPEAT);
            // 链表
            if (list_1000 > 0.001) {
                double ratio = list_10000 / list_1000;
                printf("链表总时间：1000条 %.3f ms, 10000条 %.3f ms, 增长 %.2f 倍 (理论约10)\n", list_1000, list_10000, ratio);
                if (ratio > 6 && ratio < 15) printf("   → 增长符合 O(n) 预期。\n");
                else printf("   → 偏差较大。\n");
            }
            // 哈希表：期望 O(1)，因此总时间应基本不变（增长 < 5）
            if (hash_1000 > 0.001) {
                double ratio = hash_10000 / hash_1000;
                printf("哈希表总时间：1000条 %.3f ms, 10000条 %.3f ms, 增长 %.2f 倍 (期望接近1)\n", hash_1000, hash_10000, ratio);
                if (ratio < 5.0) printf("   → 增长很小，符合 O(1) 预期。\n");
                else printf("   → 增长偏大，可能存在退化。\n");
            }
        } else if (op == 4) { // 遍历
            printf("（注：遍历所有元素，总时间与 n 成正比）\n");
            if (list_1000 > 0.001) {
                double ratio = list_10000 / list_1000;
                printf("链表遍历：1000条 %.3f ms, 10000条 %.3f ms, 增长 %.2f 倍 (理论约10)\n", list_1000, list_10000, ratio);
                if (ratio > 6 && ratio < 15) printf("   → 符合 O(n) 预期。\n");
                else printf("   → 偏差较大。\n");
            }
            if (hash_1000 > 0.001) {
                double ratio = hash_10000 / hash_1000;
                printf("哈希表遍历：1000条 %.3f ms, 10000条 %.3f ms, 增长 %.2f 倍 (理论约10)\n", hash_1000, hash_10000, ratio);
                if (ratio > 6 && ratio < 15) printf("   → 符合 O(n) 预期。\n");
                else printf("   → 偏差较大。\n");
            }
        } else if (op == 5) { // 排序
            double theory_ratio = 10000.0 * log(10000) / (1000.0 * log(1000)); // 约 13.3
            printf("理论增长倍数 ≈ %.1f\n", theory_ratio);
            if (list_1000 > 0.001) {
                double ratio = list_10000 / list_1000;
                printf("链表排序：1000条 %.3f ms, 10000条 %.3f ms, 增长 %.2f 倍\n", list_1000, list_10000, ratio);
                if (fabs(ratio - theory_ratio) / theory_ratio < 0.3) printf("   → 符合 O(n log n) 预期。\n");
                else printf("   → 偏差较大。\n");
            }
            if (hash_1000 > 0.001) {
                double ratio = hash_10000 / hash_1000;
                printf("哈希表排序：1000条 %.3f ms, 10000条 %.3f ms, 增长 %.2f 倍\n", hash_1000, hash_10000, ratio);
                if (fabs(ratio - theory_ratio) / theory_ratio < 0.3) printf("   → 符合 O(n log n) 预期。\n");
                else printf("   → 偏差较大。\n");
            }
        }
        printf("\n");
    }

    printf("【内存占用分析】\n");
    printf("链表: 100条 %.4f MB, 1000条 %.4f MB, 10000条 %.4f MB (线性增长)\n",
           memory_usage[0][0], memory_usage[1][0], memory_usage[2][0]);
    printf("哈希表: 100条 %.4f MB, 1000条 %.4f MB, 10000条 %.4f MB (线性增长 + 桶开销)\n",
           memory_usage[0][1], memory_usage[1][1], memory_usage[2][1]);

    printf("\n══════════════════════════════════════════════════════════════\n");
    printf("选型建议：\n");
    printf("   - 若频繁按关键字查找、修改、删除，哈希表是首选（平均 O(1)）。\n");
    printf("   - 若需要有序遍历、区间查询或保持插入顺序，链表更合适。\n");
    printf("   - 排序操作均需 O(n log n) 时间，与存储结构无关（取决于排序算法）。\n");
    printf("   - 内存敏感且数据量巨大时，链表比低装载因子的哈希表更节省内存。\n");
    printf("══════════════════════════════════════════════════════════════\n");
}
