#include "fs.h"
#include "ide.h"
#include "dir.h"
#include "inode.h"
#include "print.h"
#include "debug.h"
#include "string.h"
#include "super_block.h"
#include "syscall-init.h"

#define SECTOR_PER_BLOCK (BLOCK_SIZE / SECTOR_SIZE)     // 每块包含的扇区数

struct partition *cur_part;

/* 格式化分区 */
static void partition_format(struct partition *part)
{
    // 约定第0个扇区为os引导扇区，第1个扇区为超级块
    uint32_t boot_sector_sects = 1;
    uint32_t super_block_sects = 1;
    uint32_t inode_bitmap_sects = DIV_ROUND_UP(MAX_FILES_PER_PART, 8 * SECTOR_SIZE);
    uint32_t inode_table_sects = DIV_ROUND_UP(MAX_FILES_PER_PART * sizeof(struct inode), SECTOR_SIZE);
    uint32_t used_sects = boot_sector_sects + super_block_sects + inode_bitmap_sects + inode_table_sects;
    if(part->sec_cnt <= used_sects)
    {
        printk("partition %s doesn't have enough space to format file system\n", part->name);
        return;
    }
    // 剩余的扇区数(供空闲块与空闲块位图占用)，最好考虑扇区数不足的情况
    uint32_t rest_sects = part->sec_cnt - used_sects;
    uint32_t block_cnt = rest_sects / SECTOR_PER_BLOCK;
    rest_sects %= SECTOR_PER_BLOCK;
    if(block_cnt <= 1 && rest_sects == 0)
    {
        printk("partition %s doesn't have enough space to format file system\n", part->name);
        return;
    }
    uint32_t block_bitmap_sects = DIV_ROUND_UP(block_cnt, SECTOR_SIZE * 8);
    if(block_bitmap_sects > rest_sects)
    {
        uint32_t var = DIV_ROUND_UP(block_bitmap_sects - rest_sects, SECTOR_PER_BLOCK);
        block_cnt -= var;   // block_cnt一定大于var
        rest_sects = var * SECTOR_PER_BLOCK - block_bitmap_sects;
    }
    else
        rest_sects -= block_bitmap_sects;

    struct super_block sb;
    sb.magic = FS_MY_MAGIC;
    sb.sec_cnt = part->sec_cnt;
    sb.block_cnt = block_cnt;
    sb.inode_cnt = MAX_FILES_PER_PART;
    sb.lba_base = part->start_lba;
    sb.block_bitmap_lba = sb.lba_base + boot_sector_sects + super_block_sects;
    sb.block_bitmap_sects = block_bitmap_sects;
    sb.inode_bitmap_lba = sb.block_bitmap_lba + sb.block_bitmap_sects;
    sb.inode_bitmap_sects = inode_bitmap_sects;
    sb.inode_table_lba = sb.inode_bitmap_lba + sb.inode_bitmap_sects;
    sb.inode_table_sects = inode_table_sects;
    sb.block_start_lba = sb.inode_table_lba + sb.inode_table_sects + rest_sects;
    sb.root_inode_no = 0;
    sb.dir_entry_size = sizeof(struct dir_entry);
    printk("%s info:\n", part->name);
    printk("   magic:0x%x\n", sb.magic);
    printk("   part_lba_base:0x%x\n", sb.lba_base);
    printk("   sec_cnt:0x%x\n", sb.sec_cnt);
    printk("   block_cnt:0x%x\n", sb.block_cnt);
    printk("   inode_cnt:0x%x\n", sb.inode_cnt);
    printk("   block_bitmap_lba:0x%x\n", sb.block_bitmap_lba);
    printk("   block_bitmap_sects:0x%x\n", sb.block_bitmap_sects);
    printk("   inode_bitmap_lba:0x%x\n", sb.inode_bitmap_lba);
    printk("   inode_bitmap_sects:0x%x\n", sb.inode_bitmap_sects);
    printk("   inode_table_lba:0x%x\n", sb.inode_table_lba);
    printk("   inode_table_sects:0x%x\n", sb.inode_table_sects);
    printk("   block_start_lba:0x%x\n", sb.block_start_lba);
    printk("   root_inode_no:0x%x\n", sb.root_inode_no);
    printk("   dir_entry_size:0x%x\n", sb.dir_entry_size);
    // 将超级块、块位图、inode位图、inode_table写入硬盘
    // 位图与inode_table写入时需注意要为根目录占位，位图的多余位要置1
    // 最后初始化根目录(第0块)，初始值为.与..
    struct disk *hd = part->my_disk;
    uint32_t m;
    if(sb.block_bitmap_sects > sb.inode_bitmap_sects)
        m = sb.block_bitmap_sects;
    else
        m = sb.inode_bitmap_sects;
    if(m < sb.inode_table_sects)
        m = sb.inode_table_sects;
    if(m < SECTOR_PER_BLOCK)
        m = SECTOR_PER_BLOCK;
    m *= SECTOR_SIZE;
    uint8_t *buff = (uint8_t*)sys_malloc(m);        // 先申请最大的位图所占用的空间
    uint32_t v1;
    uint8_t v2, base;                               // 用于协助置位，v1代表商，v2代表余数
    // 写入超级块
    ide_write(hd, part->start_lba + 1, (void*)&sb, 1);
    printk("   super_block_lba:0x%x\n", part->start_lba + 1);
    // 写入块位图
    memset(buff, 0xFF, m);
    v1 = sb.block_cnt / 8;
    v2 = sb.block_cnt % 8;
    base = 0;
    while(v2-- > 0)
        base = (base << 1) | 1;
    memset(buff, 0, v1);
    if(base != 0)
        memset(buff + v1, base, 1);                 // 如果是0就无需写入，否则可能造成数组越界
    buff[0] |= 1;                                   // 将第0块空出，用于根目录，由于bitmap对每个字节从低位向高位设置，因此与1逻辑与就是设置第0块
    ide_write(hd, sb.block_bitmap_lba, buff, sb.block_bitmap_sects);
    // 写入inode位图
    memset(buff, 0xFF, m);
    v1 = sb.inode_cnt / 8;
    v2 = sb.inode_cnt % 8;
    base = 0;
    while(v2-- > 0)
        base = (base << 1) | 1;
    memset(buff, 0, v1);
    if(base != 0)
        memset(buff + v1, base, 1);
    buff[0] |= 1;
    ide_write(hd, sb.inode_bitmap_lba, buff, sb.inode_bitmap_sects);
    // 写入inode_table
    memset(buff, 0, m);
    struct inode *root = (struct inode*)buff;
    root->i_no = 0;
    root->i_size = 2 * sb.dir_entry_size;
    root->i_blocks[0] = sb.block_start_lba;
    ide_write(hd, sb.inode_table_lba, buff, sb.inode_table_sects);
    // 将根目录写入block起始块
    memset(buff, 0, m);
    struct dir_entry *d = (struct dir_entry*)buff;
    memcpy(d->filename, ".", 1);        // 由于已经初始化为0，因此无须担心字符串结束符
    d->i_no = 0;
    d->f_type = FT_DIRECTORY;
    ++d;
    memcpy(d->filename, "..", 2);
    d->i_no = 0;
    d->f_type = FT_DIRECTORY;
    ide_write(hd, sb.block_start_lba, buff, SECTOR_PER_BLOCK);

    printk("   root_dir_lba:0x%x\n", sb.block_start_lba);
    printk("%s format done\n", part->name);
    sys_free(buff);
}

/* 将分区名为arg的分区挂载到默认分区cur_part */
static bool mount_partition(struct list_elem *elem, int arg)
{
    struct partition *part = elem2entry(struct partition, part_tag, elem);
    if(strcmp(part->name, (char*)arg) != 0)
        return FALSE;

    cur_part = part;
    struct disk *hd = cur_part->my_disk;
    struct super_block *sb_buff = (struct super_block*)sys_malloc(SUPER_BLOCK_SECTS * SECTOR_SIZE);
    cur_part->sb = (struct super_block*)sys_malloc(sizeof(struct super_block));
    if(cur_part->sb == NULL)
        PANIC("mount_partition: alloc memory failed!\n");
    ide_read(cur_part->my_disk, cur_part->start_lba + SUPER_BLOCK_OFFSET, sb_buff, SUPER_BLOCK_SECTS);
    memcpy(cur_part->sb, sb_buff, sizeof(struct super_block));      // 复制超级块信息

    list_init(&cur_part->open_inodes);                              // 初始化打开的inode队列

    // 初始化块位图与inode位图，注意bitmap_init只负责将位图内容清零
    cur_part->block_bitmap.btmp_bytes_len = sb_buff->block_bitmap_sects * SECTOR_SIZE;
    cur_part->block_bitmap.bits = (uint8_t*)sys_malloc(cur_part->block_bitmap.btmp_bytes_len);
    if(cur_part->block_bitmap.bits == NULL)
        PANIC("mount_partition: alloc memory failed!\n");
    ide_read(hd, sb_buff->block_bitmap_lba, cur_part->block_bitmap.bits, sb_buff->block_bitmap_sects);

    cur_part->inode_bitmap.btmp_bytes_len = sb_buff->inode_bitmap_sects * SECTOR_SIZE;
    cur_part->inode_bitmap.bits = (uint8_t*)sys_malloc(cur_part->inode_bitmap.btmp_bytes_len);
    if(cur_part->inode_bitmap.bits == NULL)
        PANIC("mount_partition: alloc memory failed!\n");
    ide_read(hd, sb_buff->inode_bitmap_lba, cur_part->inode_bitmap.bits, sb_buff->inode_bitmap_sects);

    printk("mount %s done!\n", cur_part->name);
    sys_free(sb_buff);
    return TRUE;
}

void fs_init()
{
    // 扫描每个分区，如果已存在
    int partition_len = list_len(&partition_list);
    struct partition *part;
    struct list_elem *elem;
    struct super_block *sb = (struct super_block*)sys_malloc(sizeof(struct super_block));
    while(partition_len-- > 0)
    {
        elem = list_pop(&partition_list);
        part = elem2entry(struct partition, part_tag, elem);
        // 保险起见跳过主盘
        if(part->my_disk->dev_no == 0)
            continue;
        ide_read(part->my_disk, part->start_lba + 1, (void*)sb, 1);
        if(sb->magic == FS_MY_MAGIC)
            printk("%s has filesystem\n", part->name);
        else
        {
            printk("formatting partition %s......\n", part->name);
            partition_format(part);
        }
        list_append(&partition_list, elem);
    }
    list_traversal(&partition_list, mount_partition, (int)"sdb1");
    sys_free(sb);
}
