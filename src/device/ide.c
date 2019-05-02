#include "ide.h"
#include "io.h"
#include "timer.h"
#include "print.h"
#include "stdio.h"
#include "debug.h"
#include "syscall-init.h"
#include "interrupt.h"

/* 硬盘各寄存器端口号 */
#define reg_data(channel) (channel->port_base + 0)
#define reg_error(channel) (channel->port_base + 1)
#define reg_sect_cnt(channel) (channel->port_base + 2)
#define reg_lba_l(channel) (channel->port_base + 3)
#define reg_lba_m(channel) (channel->port_base + 4)
#define reg_lba_h(channel) (channel->port_base + 5)
#define reg_dev(channel) (channel->port_base + 6)
#define reg_status(channel) (channel->port_base + 7)
#define reg_cmd(channel) reg_status(channel)
#define reg_alt_status(channel) (channel->port_base + 0x206)
#define reg_ctl(channel) reg_alt_status(channel)

/* status寄存器的关键位 */
#define BIT_STAT_BSY 0x80       // 硬盘忙
#define BIT_STAT_DRDY 0x40      // 驱动器已准备好
#define BIT_STAT_DRQ 0x8        // 数据传输已准备好

/* device寄存器的关键位 */
#define BIT_DEV_MBS 0xa0        // 第7位和第5位固定为1
#define BIT_DEV_LBA 0x40        // LBA模式
#define BIT_DEV_DEV 0x10        // 从盘

/* 硬盘操作指令 */
#define CMD_IDENTIFY 0xec       // identify
#define CMD_READ_SECTOR 0x20    // 读扇区
#define CMD_WRITE_SECTOR 0x30   // 写扇区

/* 可读写的最大扇区数 */
#define max_lba ((80*1024*1024/512) - 1)    // 80M硬盘

uint8_t channel_cnt;            // 通道数
struct ide_channel channels[2]; // 两个ide通道

static uint32_t ext_lba_base = 0;       // 总扩展分区起始lba
static uint8_t d_no = 0;                // 硬盘下标
static uint8_t p_no = 0;                // 主分区下标
static uint8_t l_no = 0;                // 逻辑分区下标
struct list partition_list;             // 分区队列

struct partition_table_entry
{
    uint8_t bootable;           // 是否可引导
    uint8_t start_head;         // 起始磁头号
    uint8_t start_sec;          // 起始扇区号
    uint8_t start_chs;          // 起始柱面号
    uint8_t fs_type;            // 分区类型
    uint8_t end_head;           // 结束磁头号
    uint8_t end_sec;            // 结束扇区号
    uint8_t end_chs;            // 结束柱面号
    uint32_t start_lba;         // 分区起始lba
    uint32_t sec_cnt;           // 分区扇区数
} __attribute__((packed));      // 保证该结构体为16B

struct boot_sector
{
    uint8_t boot_code[446];                             // 引导代码
    struct partition_table_entry partition_table[4];    // 分区表
    uint16_t signature;                                 // 分区结束标志
} __attribute__((packed));

/* 选择要读写的硬盘，并写入硬盘所在通道的device寄存器 */
static void select_disk(struct disk *hd)
{
    uint8_t device = BIT_DEV_MBS | BIT_DEV_LBA;
    if(hd->dev_no)
        device |= BIT_DEV_DEV;
    outb(reg_dev(hd->my_channel), device);
}

/* 向硬盘控制器写入起始lba地址以及要读写的扇区数 */
static void select_sector(struct disk *hd, uint32_t lba, uint8_t sec_cnt)
{
    ASSERT(lba < max_lba);
    struct ide_channel *channel = hd->my_channel;
    // 如果sec_cnt为0，则表示写入256
    outb(reg_sect_cnt(channel), sec_cnt);
    outb(reg_lba_l(channel), lba);
    outb(reg_lba_m(channel), lba >> 8);
    outb(reg_lba_h(channel), lba >> 16);
    outb(reg_dev(channel), BIT_DEV_MBS | BIT_DEV_LBA | (hd->dev_no == 1 ? BIT_DEV_DEV : 0) | lba >> 24);
}

/* 向通道发出控制指令 */
static void cmd_out(struct ide_channel *channel, uint8_t cmd)
{
    channel->expecting_intr = TRUE;
    outb(reg_cmd(channel), cmd);
}
/* 从硬盘读取sec_cnt个扇区到buff */
static void read_from_sector(struct disk *hd, void *buff, uint8_t sec_cnt)
{
    uint32_t bytes_cnt = sec_cnt == 0 ? 256 * BYTES_PER_SECTOR : sec_cnt * BYTES_PER_SECTOR;
    insw(reg_data(hd->my_channel), buff, bytes_cnt / 2);
}

/* 将buff中sec_cnt个扇区大小的数据写入硬盘 */
static void write2sector(struct disk *hd, void *buff, uint8_t sec_cnt)
{
    uint32_t byte_cnt = sec_cnt == 0 ? 256 * BYTES_PER_SECTOR : sec_cnt * BYTES_PER_SECTOR;
    outsw(reg_data(hd->my_channel), buff, byte_cnt / 2);
}

/* 根据ata手册硬盘操作应在31秒内完成，因此等待30秒，返回1表示可读写 */
static bool busy_wait(struct ide_channel *channel)
{
    uint32_t time_limit = 30 * 1000;    // 30000ms = 30s
    while(time_limit -= 10 >= 0)
    {
        if(!(inb(reg_status(channel)) & BIT_STAT_BSY))
            return inb(reg_status(channel)) & BIT_STAT_DRQ;
        mtime_sleep(10);
    }
    return FALSE;
}

/* 读取硬盘hd以lba为起始的连续sec_cnt个扇区到buff */
void ide_read(struct disk *hd, uint32_t lba, void *buff, uint32_t sec_cnt)
{
    ASSERT(lba < max_lba);
    uint32_t sec_do, sec_left = sec_cnt;
    struct ide_channel *channel = hd->my_channel;
    lock_acquire(&channel->lock);
    select_disk(hd);
    while(sec_left)
    {
        // 由于一次最多设置读256个扇区，因此需分段读入
        if(sec_left >= 256)
            sec_do = 256;
        else
            sec_do = sec_left;
        select_sector(hd, lba, sec_do);
        cmd_out(channel, CMD_READ_SECTOR);
        sema_down(&channel->disk_done);
        if(!busy_wait(channel))
        {
            char error[64];
            sprintf(error, "%s read sector %d failed!\n", hd->name, lba);
            PANIC(error);
        }
        read_from_sector(hd, buff, sec_do);
        // 完成读入后，需更新lba、sec_left、buff
        lba += sec_do;
        sec_left -= sec_do;
        buff = (void*)((uint32_t)buff + sec_do * BYTES_PER_SECTOR);
    }
    lock_release(&channel->lock);
}

/* 将buff写入硬盘hd以lba为起始的连续sec_cnt个扇区处 */
void ide_write(struct disk *hd, uint32_t lba, void *buff, uint32_t sec_cnt)
{
    ASSERT(lba < max_lba);
    uint32_t sec_do, sec_left = sec_cnt;
    struct ide_channel *channel = hd->my_channel;
    lock_acquire(&channel->lock);
    select_disk(hd);
    while(sec_left)
    {
        // 由于一次最多设置写256个扇区，因此需分段写入
        if(sec_left >= 256)
            sec_do = 256;
        else
            sec_do = sec_left;
        select_sector(hd, lba, sec_do);
        cmd_out(channel, CMD_WRITE_SECTOR);
        if(!busy_wait(channel))
        {
            char error[64];
            sprintf(error, "%s read sector %d failed!\n", hd->name, lba);
            PANIC(error);
        }
        write2sector(hd, buff, sec_do);
        sema_down(&channel->disk_done);     // 为什么要放在这里?
        // 完成读入后，需更新lba、sec_left、buff
        lba += sec_do;
        sec_left -= sec_do;
        buff = (void*)((uint32_t)buff + sec_do * BYTES_PER_SECTOR);
    }
    lock_release(&channel->lock);
}

static void swap_pairs_bytes(const char *dst, char *buff, uint32_t len)
{

}

/* 读取硬盘hd的信息 */
static void identify_disk(struct disk *hd)
{
    char id_info[512];
    select_disk(hd);
    cmd_out(hd->my_channel, CMD_IDENTIFY);
    sema_down(&hd->my_channel->disk_done);
    if(!busy_wait(hd->my_channel))
    {
        char error[64];
        sprintf(error, "%s identify failed!\n", hd->name);
        PANIC(error);
    }
    read_from_sector(hd, id_info, 1);
    uint32_t sectors = *(uint32_t*)&id_info[60*2];
    printk("SECTORS: %d\n", sectors);
    printk("CAPACITY: %dMB\n", sectors * 512 / 1024 / 1024);
    hd->sectors = sectors;
}

/* 扫描硬盘hd中地址为ext_lba扇区中的所有分区，总扩展分区起始lba不为0时，表明此函数正在扫描子扩展分区，扫描分区前，需要给
   出被扫描分区上一级的起始lba地址，并将其存放于ext_lba_base中，此函数只扫描可识别的分区，并将其链入partition_list */
static void partition_scan(struct disk *hd, uint32_t ext_lba)
{
    struct boot_sector *bs = (struct boot_sector*)sys_malloc(sizeof(struct boot_sector));
    ide_read(hd, ext_lba, (void*)bs, 1);
    uint8_t i;
    struct partition_table_entry *p = bs->partition_table;
    struct partition *part = ext_lba_base == 0 ? hd->prim_parts : hd->logic_parts;
    // 对该分区表的每个表项遍历
    for(i = 0 ; i < 4 ; ++i, ++p)
    {
        /* 分区文件类型或启动类型不可识别时跳过 */
        if(p->fs_type == FS_INVALID || (p->bootable != BOOTABLE && p->bootable != NO_BOOTABLE))
            continue;
        if(p->fs_type == FS_EXTENDED)
        {
            // 若该分区是主扩展分区，则记录其起始lba地址
            // 由于所有的主分区都在一张分区表内，因此无需担心此处对ext_lba_base的修改会造成其他主分区的信息记录错误(lba_base与part变量依赖ext_lba_base)
            if(ext_lba_base == 0)
            {
                ext_lba_base = p->start_lba;
                partition_scan(hd, p->start_lba);
            }
            else
                partition_scan(hd, p->start_lba + ext_lba_base);
            // 扩展分区起始地址给出的是子扩展分区mbr地址，因此不作为分区被记录
            continue;
        }
        // 分区的起始lba等于lba偏移量+上一级分区的起始lba地址
        if(ext_lba_base == 0)
        {
            part[p_no].start_lba =  p->start_lba + ext_lba;
            part[p_no].sec_cnt = p->sec_cnt;
            part[p_no].my_disk = hd;
            if(!have_elem(&partition_list, &part[p_no].part_tag))
                list_append(&partition_list, &part[p_no].part_tag);
            sprintf(part[p_no].name, "%s%d", hd->name, p_no + 1);
            ++p_no;
            ASSERT(p_no <= 4);
        }
        else
        {
            part[l_no].start_lba =  p->start_lba + ext_lba;
            part[l_no].sec_cnt = p->sec_cnt;
            part[l_no].my_disk = hd;
            if(!have_elem(&partition_list, &part[l_no].part_tag))
                list_append(&partition_list, &part[l_no].part_tag);
            if(ext_lba_base)
                sprintf(part[l_no].name, "%s%d", hd->name, l_no + 5);
            ++l_no;
            if(l_no >= 8)
                return;
        }
        /* 分区超级块等文件系统的信息未添加 */
    }
    sys_free(bs);
}

static bool partition_info(struct list_elem *elem, int arg __attribute__((unused)))
{
    struct partition *part = elem2entry(struct partition, part_tag, elem);
    printk("   %s start_lba:%d, sec_cnt:%d\n", part->name, part->start_lba, part->sec_cnt);
    return FALSE;
}

/* 初始化通道的硬盘信息 */
static void disk_init(struct disk *hd, struct ide_channel *channel, uint8_t cnt)
{
    int i;
    for(i = 0 ; i < 2 ; ++i)
    {
        sprintf(hd[i].name, "sd%c", 'a' + d_no);
        hd[i].my_channel = channel;
        hd[i].dev_no = i;
        // 由于总扩展分区起始lba为全局变量，每次扫描分区时需重置
        p_no = l_no = ext_lba_base = 0;
        partition_scan(&hd[i], 0);
        identify_disk(&hd[i]);
        d_no++;
    }
}

/* 硬盘中断处理程序，用以唤醒阻塞于硬盘读写的线程/进程 */
void intr_hd_handler(uint8_t irq_no)
{
    ASSERT(irq_no == 0x2e || irq_no == 0x2f);
    uint8_t ch_no = irq_no - 0x2e;
    struct ide_channel *channel = &channels[ch_no];
    ASSERT(channel->irq_no == irq_no);
    // 对同一通道的访问由通道锁控制，读写硬盘之前会申请通道锁，因此无需关心此次中断是否对应这一次的expecting_intr
    if(channel->expecting_intr)
    {
        channel->expecting_intr = FALSE;
        sema_up(&channel->disk_done);
        // 读硬盘的状态寄存器，使硬盘可以执行下一次读写命令
        inb(reg_status(channel));
    }
}

/* 初始化ide界面 */
void ide_init()
{
    printk("ide_init start\n");
    uint8_t hd_cnt = *(uint8_t*)0x475;  // 0x475位于bios数据区，保存了硬盘的数量
    ASSERT(hd_cnt > 0);
    channel_cnt = hd_cnt % 2 ? hd_cnt/2+1 : hd_cnt/2;
    struct ide_channel *channel;
    uint8_t channel_no = 0;
    // 初始化分区队列
    list_init(&partition_list);
    // 初始化每个通道的信息
    while(channel_no < channel_cnt)
    {
        channel = &channels[channel_no];
        sprintf(channel->name, "ide%d", channel_no);
        switch(channel_no)
        {
            case 0:
                channel->port_base = 0x1f0;
                channel->irq_no = 0x20 + 14;
                break;
            case 1:
                channel->port_base = 0x170;
                channel->irq_no = 0x20 + 15;
                break;
        }
        channel->expecting_intr = FALSE;
        lock_init(&channel->lock);
        sema_init(&channel->disk_done, 0);
        // 为每个通道注册硬盘的中断处理函数
        register_handler(channel->irq_no, intr_hd_handler);
        // 由于disk_init需要用到硬盘中断，因此必须在注册中断处理程序后调用
        disk_init(channel->devices, channel, 2);
        ++channel_no;
    }
    list_traversal(&partition_list, partition_info, (int)NULL);
    printk("ide_init done\n");
}
