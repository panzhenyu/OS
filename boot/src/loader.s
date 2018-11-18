%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR

	jmp loader_start

;构建gdt以及其内部的描述符
	GDT_BASE:	dd 0x00000000
			dd 0x00000000			; gdt表第零项空出，从第一项开始放入段描述符

	CODE_DESC:	dd 0x0000ffff
			dd DESC_CODE_HIGH4		; 小端，因此高四字节放高地址

	DATA_STACK_DESC:dd 0x0000ffff
			dd DESC_DATA_HIGH4

	VIDEO_DESC:	dd 0x80000007			; 显存段描述符，limit=(0xbffff-0xb8000)/4k=0x7
			dd DESC_VIDEO_HIGH4

	GDT_SIZE equ $ - GDT_BASE
	GDT_LIMIT equ GDT_SIZE - 1
	times 60 dq 0					; 预留60个段描述符的空位

	SELECTOR_CODE equ (0x0001 << 3) + TI_GDT + RPL0	; 代码段选择子
	SELECTOR_DATA equ (0x0002 << 3) + TI_GDT + RPL0
	SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0

	total_mem_bytes dd 0				; 用于保存内存总容量

;以下是gdt的指针，前2字节是gdt界限，后4字节是gdt起始地址

	gdt_ptr dw GDT_LIMIT
		dd GDT_BASE

	ards_buf times 244 db 0				; 人工对齐,total_mem_bytes->ards_nr共256字节
	ards_nr dw 0					; 用于记录ARDS结构体数量

	loadermsg db '2 loader in real.'

loader_start:
;int 0x15 EAX=0x0000E820 EDX=534D4150h('SMAP') 获得内存布局
	xor ebx, ebx					; 第一次调用时，EBX要为0
	mov edx, 0x534d4150
	mov di, ards_buf
.e820_mem_get_loop:
	mov eax, 0x0000e820
	mov ecx, 20					; ARDS描述符大小为20字节
	int 0x15
	jc .e820_failed_so_try_e801
	add di, cx					; 使di缓冲区增加20字节指向缓冲区中新的ARDS结构位置
	inc word [ards_nr]
	cmp ebx, 0					; 若ebx为0且cf不为1，说明ards全部返回
	jnz .e820_mem_get_loop
;在所有ards结构中，找出[base_add_low + length_low]的最大值，即内存的容量
	mov cx, [ards_nr]
	mov ebx, ards_buf
	xor edx, edx					; 用于保存最大内存容量
.find_max_mem_area:
	mov eax, [ebx]					; base_add_low
	add eax, [ebx+8]				; length_low
	add ebx, 20
	cmp edx, eax
	jge .next_ards
	mov edx, eax
.next_ards:
	loop .find_max_mem_area
	jmp .mem_get_ok

;int 0x15 AX=0xE801 获取内存大小，最大支持4G
;返回后，AX与CX相等，以1KB为单位，BX和DX相等，以64KB为单位
.e820_failed_so_try_e801:
;1 计算低15M内存
	mov eax, 0xe801
	int 0x15
	jc .e801_failed_so_try_88
	mov cx, 0x400					; 单位转换为byte，乘以1k
	mul cx
	shl edx, 16
	and eax, 0x0000ffff
	or edx, eax
	add edx, 0x100000				; ax只是15M,因此增加1M
	mov esi, edx					; 备份
;2 计算16M以上的内存
	xor eax, eax
	mov ax, bx
	mov ecx, 0x10000				; 单位转换为byte，乘以64k
	mul ecx
	add esi, eax
	mov edx, esi
	jmp .mem_get_ok

;int 0x15 AX=0x88 获取内存大小，最大支持64M
.e801_failed_so_try_88:
	mov ah, 0x88
	int 0x15
	jc .error_hlt
	and eax, 0x0000ffff
	mov cx, 0x400
	mul cx						; 16位乘法，高16位位于dx，低16位位于ax
	shl edx, 16
	or edx, eax
	add edx, 0x100000
	jmp .mem_get_ok

.error_hlt:
	hlt

.mem_get_ok:
	mov [total_mem_bytes], edx			; 将内存大小(单位:byte)存入total_mem_bytes处

;打开A20
	in al, 0x92
	or al, 0000_0010B
	out 0x92, al

;加载gdt
	lgdt [gdt_ptr]

;crt0的第0位置1
	mov eax, cr0
	or eax, 0x00000001
	mov cr0, eax

	jmp dword SELECTOR_CODE:p_mode_start		; 刷新流水线


[bits 32]
p_mode_start:
	mov ax, SELECTOR_DATA
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov esp, LOADER_STACK_TOP
	mov ax, SELECTOR_VIDEO
	mov gs, ax					; 设置显存段寄存器为显存段选择子
	mov byte [gs:160], 'P'

	jmp $

;创建页目录及页表，清零页目录项的所占的内存区，设置了第一个页目录项和内核区页目录项，将0-1M内存地址写入第一张页表
setup_page:
	mov ecx, 4096
	mov esi, 0
.clear_page_dir:
	mov byte [PAGE_DIR_TABLE_POS + esi], 0
	inc esi
	loop .clear_page_dir

;创建页目录项(PDE)
.create_pde:
	mov eax, PAGE_DIR_TABLE_POS
	add eax, 0x1000					; 第一个页表位置及属性
	mov ebx, eax					; 为create_pte做准备，ebx为基地址

	or eax, PG_US_U | PG__RW_W | PG_P
	mov [PAGE_DIR_TABLE_POS + 0x0], eax		; 第一个目录项，此时指向第一个页表
	mov [PAGE_DIR_TABLE_POS + 0xc00], eax		; 内核空间的起始目录项，此时指向第一个页表
	sub eax, 0x1000
	mov [PAGE_DIR_TABLE_POS + 4092], eax		; 使最后一个目录项指向页目录表自己的地址

;创建页表项(PTE)
	mov ecx, 256					; 1M低端内存 / 4K = 256
	mov esi, 0
	mov edx, PG_US_U | PG_RW_W | PG_P
.create_pte:
	mov [ebx+esi*4], edx
	add edx, 0x1000
	inc esi
	loop .create_pte

;创建内核以其他页表的pde
	mov eax, PAGE_DIR_TABLE_POS
	add eax, 2000					; 第二个页表位置及属性
	or eax, PG_US_U | PG_RW_W | PG_P
	mov ebx, PAGE_DIR_TABLE_POS
	mov ecx, 254					; 由于最后一个页目录项是页目录首项，且内核空间起始目录项已指向第一个页表，因此为256-2=254
	mov esi, 769					; 从内核空间的第二个页目录项开始
.create_kernel_pde:
	mov [ebx+esi*4], eax
	inc esi
	add eax, 0x1000					; 每张页表为4K大小
	loop .create_kernel_pde
	ret
