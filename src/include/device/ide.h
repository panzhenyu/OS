#ifndef _DEVICE_IDE_H
#define _DEVICE_IDE_H

#include "global.h"
#include "list.h"
#include "sync.h"
#include "bitmap.h"

/* 分区表字段意义 */
#define BOOTABLE 0x80
#define NO_BOOTABLE 0
#define FS_INVALID 0
#define FS_EXTENDED 5

#define MAX_PRIM_PARTS 4
#define MAX_LOGIC_PARTS 8
#define BYTES_PER_SECTOR 512

struct partition
{
    uint32_t start_lba;         // 起始扇区
    uint32_t sec_cnt;           // 扇区数
    struct disk *my_disk;       // 分区所属的硬盘
    struct list_elem part_tag;  // 用于队列标记
    char name[8];               // 分区名称
    struct super_block *sb;     // 本分区超级块
    struct bitmap block_bitmap; // 块位图
    struct bitmap inode_bitmap; // inode位图
    struct list open_inodes;    // 本分区打开的inode队列
};

struct disk
{
    char name[8];                                   // 硬盘名称
    struct ide_channel *my_channel;                 // 硬盘归属的ide通道
    uint8_t dev_no;                                 // 主盘为0，从盘为1
    struct partition prim_parts[MAX_PRIM_PARTS];    // 主分区，最多4个
    struct partition logic_parts[MAX_LOGIC_PARTS];  // 逻辑分区，最多8个
};

struct ide_channel
{
    char name[8];                   // 本ata通道名称
    uint16_t port_base;             // 本通道的起始端口号
    uint8_t irq_no;                 // 本通道所用的中断号
    struct lock lock;               // 通道锁
    bool expecting_intr;            // 表示等待硬盘中断
    struct semaphore disk_done;     // 用于阻塞、唤醒驱动程序
    struct disk devices[2];         // 一个通道上连接主盘和从盘
};

void ide_read(struct disk *hd, uint32_t lba, void *buff, uint32_t sec_cnt);
void ide_write(struct disk *hd, uint32_t lba, void *buff, uint32_t sec_cnt);
void ide_init();

#endif
