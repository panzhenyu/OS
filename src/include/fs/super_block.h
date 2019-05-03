#ifndef _FS_SUPER_BLOCK_H
#define _FS_SUPER_BLOCK_H

#include "stdint.h"

/* 超级块 */
struct super_block
{
    uint32_t magic;                 // 文件系统魔数
    uint32_t sec_cnt;               // 本分区扇区数
    uint32_t block_cnt;             // 本分区空闲块数
    uint32_t inode_cnt;             // 本分区inode数
    uint32_t lba_base;              // 本分区起始lba

    uint32_t block_bitmap_lba;      // 块位图起始lba
    uint32_t block_bitmap_sects;    // 块位图占用的扇区数

    uint32_t inode_bitmap_lba;      // inode位图起始lba
    uint32_t inode_bitmap_sects;    // inode位图占用的扇区数

    uint32_t inode_table_lba;       // inode数组起始lba
    uint32_t inode_table_sects;     // inode数组占用的扇区数

    uint32_t block_start_lba;       // 空闲块起始lba
    uint32_t root_inode_no;         // 根目录inode结点号
    uint32_t dir_entry_size;        // 目录项大小

    uint8_t pad[456];
} __attribute__((packed));          // super_block占512字节，刚好为一个扇区


#endif