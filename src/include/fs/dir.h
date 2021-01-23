#ifndef _FS_DIR_H
#define _FS_DIR_H

#include "inode.h"
#include "fs.h"

#define MAX_FILE_NAME_LEN 32    // 最大文件名长度

/* 目录 */
struct dir
{
    struct inode *inode;
    uint32_t dir_pos;           // 记录在目录内的偏移
    uint8_t dir_buf[512];       // 目录的数据缓存
};

/* 目录项 */
struct dir_entry
{
    char filename[MAX_FILE_NAME_LEN];   // 文件名
    uint32_t i_no;                      // inode编号
    enum file_types f_type;             // 文件类型
};

#endif