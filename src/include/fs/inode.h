#ifndef _FS_INODE_H
#define _FS_INODE_H

#include "global.h"
#include "list.h"

/* inode */
struct inode
{
    uint32_t i_no;                  // inode编号
    uint32_t i_size;                // inode是文件时表示文件大小，是目录时表示目录项大小之和
    uint32_t i_open_cnts;           // 此文件被打开的次数
    bool write_deny;                // 写锁
    uint32_t i_blocks[13];          // s前12块为数据块，第13块为一级索引块
    struct list_elem inode_tag;     // 用于链入已打开的inode队列
};

#endif