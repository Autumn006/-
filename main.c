/*********************************************************************
 * 文件名：main.c
 * 说明：  程序入口，提供交互式菜单，管理选课记录。
 *         使用双向链表或哈希表存储数据。
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "adt.h"
#include "util.h"

int main() {
    srand((unsigned int)time(NULL));

    DataStructure ds;
    ds_init(&ds, 0);   /* 默认使用链表 */

    /* 尝试从文件加载已有记录 */
    EnrollmentRecord *loaded_data = NULL;
    int loaded_count = 0;
    if (load_from_csv("records.csv", &loaded_data, &loaded_count)) {
        for (int i = 0; i < loaded_count; i++) {
            ds.insert(ds.storage, loaded_data[i]);
        }
        free(loaded_data);
        printf("已从文件加载 %d 条记录。\n", loaded_count);
    } else {
        printf("未找到记录文件，使用空库。\n");
    }

    int choice;
    while (1) {
        printf("\n====== 校园选课管理系统 ======\n");
        printf("当前存储结构: %s\n", ds.type == 0 ? "双链表" : "哈希表");
        printf("1. 切换存储结构\n");
        printf("2. 添加选课记录\n");
        printf("3. 修改成绩\n");
        printf("4. 查找记录\n");
        printf("5. 删除记录\n");
        printf("6. 显示所有记录(前20条)\n");
        printf("7. 条件筛选与多关键字排序\n");
        printf("8. 统计分析\n");
        printf("9. 清理过期记录\n");
        printf("10. 综合性能对比测试（含复杂度分析）\n");
        printf("11. 批量生成随机记录\n");
        printf("0. 退出并保存\n");
        printf("请输入选择: ");

        if (scanf("%d", &choice) != 1) {
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();

        switch (choice) {
            case 1: {
                int new_type = (ds.type == 0) ? 1 : 0;
                printf("将切换为 %s，确认？(y/n): ",
                       new_type == 0 ? "双链表" : "哈希表");
                char confirm;
                scanf("%c", &confirm);
                clear_input_buffer();
                if (confirm == 'y' || confirm == 'Y') {
                    ds_convert(&ds, new_type);
                    printf("已切换为 %s。\n", new_type == 0 ? "双链表" : "哈希表");
                } else {
                    printf("已取消。\n");
                }
                break;
            }
            case 2: add_record(&ds); break;
            case 3: modify_score(&ds); break;
            case 4: search_record(&ds); break;
            case 5: delete_record(&ds); break;
            case 6: list_all_records(&ds); break;
            case 7: filter_and_sort(&ds); break;
            case 8: statistics_menu(&ds); break;
            case 9: clean_expired(&ds); break;
            case 10:
                comprehensive_performance_test();
                break;   // 务必加上 break，避免掉入 case 11
            case 11:
                batch_generate(&ds);
                break;
            case 0: {
                EnrollmentRecord *all = NULL;
                int n = 0;
                ds.get_all_records(ds.storage, &all, &n);
                save_to_csv("records.csv", all, n);
                free(all);
                printf("数据已保存到 records.csv。\n");
                ds_destroy(&ds);
                printf("再见！\n");
                return 0;
            }
            default:
                printf("无效选择，请重新输入。\n");
        }
    }
    return 0;
}
