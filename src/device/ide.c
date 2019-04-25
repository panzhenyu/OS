#include "ide.h"
#include "io.h"
#include "timer.h"
#include "print.h"
#include "stdio.h"
#include "debug.h"
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
static void read_from_sector(struct disk *hd, void *buff, uint8_t sec_cnt);
{
    uint32_t bytes_cnt = sec_cnt == 0 ? 256 * BYTES_PER_SECTOR : sec_cnt * BYTES_PER_SECTOR;
    insw(reg_data(hd->my_channel), buf, bytes_cnt / 2);
}

/* 将buff中sec_cnt个扇区大小的数据写入硬盘 */
static void write2sector(struct disk *hd, void *buff, uint8_t sec_cnt);
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
        sec_do = sec_left >= 256 ? 256 : sec_left;
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
void ide_write(struct disk *hd, uint32_t _lba, void *_buff, uint32_t sec_cnt)
{
}

/* 硬盘中断处理程序，用以唤醒阻塞于硬盘读写的线程/进程 */
void intr_hd_handler(uint8_t irq_no)
{
    ASSERT(irq_no == 0x2e || irq_no == 0x2f);
    uint8_t ch_no = irq_no - 0x2e;
    struct ide_channel *channel = &channels[ch_no];
    ASSERT(channel->irq_no == ch_no);
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
        ++channel_no;
    }
    printk("ide_init done\n");
}
