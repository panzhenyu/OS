#ifndef _FS_H
#define _FS_H

/*
 ****************************** 文件系统参数 ******************************
 * 文件系统结构: 引导程序、超级块、块位图、inode位图、inode_table、空闲扇区、空闲块
 * 文件系统魔数: 0x19590318
 * 引导程序大小: 512B
 * 超级块大小: 512B(固定大小，若超过512B则需变更格式化程序)
 * 备注:空闲扇区不使用，因此超级块中的block_start_lba字段会跳过这一部分，空闲块终止
 * lba也就是该分区的终止lba
 ************************************************************************
 */

#include "global.h"

#define MAX_FILES_PER_PART 4096     // 每个分区支持的文件数
#define BITS_PER_SECTOR 4096        // 扇区位数
#define SECTOR_SIZE 512             // 扇区字节大小
#define BLOCK_SIZE 4096             // 块字节大小
#define FS_MY_MAGIC 0x19590318      // 本文件系统魔数
#define SUPER_BLOCK_OFFSET 1        // 超级块在分区中的偏移扇区
#define SUPER_BLOCK_SECTS 1         // 超级块所占的扇区数

/* 文件类型 */
enum file_types
{
    FT_UNKNOWN,     // 未知文件类型
    FT_REGULAR,     // 普通文件
    FT_DIRECTORY    // 目录文件
};

void fs_init();

#endif