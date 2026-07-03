#ifndef UTIL_H
#define UTIL_H

#include "adt.h"

/* 输入缓冲区清理 */
void clear_input_buffer(void);

/* CSV 文件读写 */
int load_from_csv(const char *filename, EnrollmentRecord **data, int *count);
void save_to_csv(const char *filename, const EnrollmentRecord *data, int count);

/* 随机生成记录 */
void generate_records(EnrollmentRecord *records, int count);

/* 交互菜单功能 */
void add_record(DataStructure *ds);
void modify_score(DataStructure *ds);
void search_record(DataStructure *ds);
void delete_record(DataStructure *ds);
void list_all_records(DataStructure *ds);
void filter_and_sort(DataStructure *ds);
void clean_expired(DataStructure *ds);
void batch_generate(DataStructure *ds);

/* 综合性能测试（合并原 case10 和 case12，含复杂度验证） */
void comprehensive_performance_test(void);

/* 统计分析 */
void course_enrollment_stats(DataStructure *ds);
void student_credits_stats(DataStructure *ds);
void college_distribution_stats(DataStructure *ds);
void semester_stats(DataStructure *ds);
void score_distribution_stats(DataStructure *ds);
void statistics_menu(DataStructure *ds);

#endif /* UTIL_H */
