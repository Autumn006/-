/*********************************************************************
 * 文件名：hash.h
 * 作者：  郭佳鑫 
 * 功能简述：
 *   声明哈希表存储的初始化接口。
 *********************************************************************/

#ifndef HASH_H
#define HASH_H

#include "adt.h"

/**
 * 将 DataStructure 绑定为哈希表（链地址法）实现
 * @param ds 待初始化的 DataStructure 指针
 */
void hash_attach(DataStructure *ds);

#endif /* HASH_H */
