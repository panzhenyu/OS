;主引导程序
%include "boot.inc"
SECTION MBR vstart=0x7c00
	mov ax, cs			; cx为0，BIOS通过0:0x7c00跳转到MBR入口处
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov fs, ax
	mov sp, 0x7c00			; 栈地址不断减小，目前0x7c00以下是安全区，因此将栈顶地址设为0x7c00
	mov ax, 0xb800			; 显存起始地址
	mov gs, ax

;INT 0x10	功能号:0x06	功能描述:上卷窗口
;input:
;AH 功能号 = 0x06
;AL = 上卷的行数(若为0，表示全部)
;BH = 上卷行属性
;(CL,CH) = 窗口左上角的(X,Y)位置
;(DL,DH) = 窗口右下角的(X,Y)位置
;无返回值
	mov ax, 0x0600
	mov bx, 0x0700
	mov cx, 0			; 左上角
	mov dx, 0x184f			; 右下角(80,25)
	int 0x10

;输出绿色背景，前景色红色，并且跳动的字符串"1 MBR"(改写为loop试试？)
	mov byte [gs:0x00], '1'
	mov byte [gs:0x01], 0xa4	; A表示绿色背景闪烁，4表示前景色为红色
	mov byte [gs:0x02], ' '
	mov byte [gs:0x03], 0xa4
	mov byte [gs:0x04], 'M'
	mov byte [gs:0x05], 0xa4
	mov byte [gs:0x06], 'B'
	mov byte [gs:0x07], 0xa4
	mov byte [gs:0x08], 'R'
	mov byte [gs:0x09], 0xa4

	mov eax, LOADER_START_SECTOR	; 起始扇区lba地址
	mov bx, LOADER_BASE_ADDR	; 写入的地址
	mov cx, 4			; 待读入的扇区数
	call rd_disk_m_16		; 读取程序的起始部分

	jmp LOADER_BASE_ADDR
	
;读取硬盘n个扇区
rd_disk_m_16:
	mov esi, eax			; 备份eax
	mov di, cx			; 备份cx
;读写硬盘
;1:设置要读取的扇区数
	mov dx, 0x1f2
	mov al, cl
	out dx, al			; 读取的扇区数
	mov eax, esi			; 恢复eax
;2:将lba地址存入0x1f3 ~ 0x1f6
	mov cl, 8
	mov dx, 0x1f3
	out dx, al

	shr eax, cl
	mov dx, 0x1f4
	out dx, al

	shr eax, cl
	mov dx, 0x1f5
	out dx, al

	shr eax, cl
	and al, 0x0f			; lba第24-27位，即device的0-3位
	or al, 0xe0			; 设置7-4位为1110，表示lba模式
	mov dx, 0x1f6
	out dx, al
;3:向0x1f7端口写入读命令，0x20
	mov dx, 0x1f7
	mov al, 0x20
	out dx, al
;4:检测硬盘状态
.not_ready:
	;同一端口，写时表示写入命令字，读时表示读入硬盘状态
	nop
	in al, dx
	and al, 0x88			; 第3位为1表示硬盘控制器已经准备好数据传输，第7位为1表示硬盘忙
	cmp al, 0x08
	jnz .not_ready			; 未准备好则继续等待
;5:从0x1f0端口读入数据
	mov ax, di			; di为要读的扇区数，一个扇区为512B，每次读入一个字，因此每个扇区需要读256次
	mov dx, 256
	mul dx
	mov cx, ax
	mov dx, 0x1f0
.go_on_read:
	in ax, dx
	mov [bx], ax
	add bx, 2
	loop .go_on_read
	ret

	times 510-($-$$) db 0
	db 0x55, 0xaa
