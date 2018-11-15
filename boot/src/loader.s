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

;以下是gdt的指针，前2字节是gdt界限，后4字节是gdt起始地址

	gdt_ptr dw GDT_LIMIT
		dd GDT_BASE
	loadermsg db '2 loader in real.'

loader_start:
;INT 0x10 功能号:0x13 功能:打印字符串
;输入:
;AH 子功能号=0x13, BH=页码, BL=属性(若AL=0x00或0x01), CX=字符串长度, (DH,DL)=坐标(行,列), ES:BP=字符串地址
;AL=显示输出方式
;	0---字符串中只含显示字符，其显示属性在BL中，显示后，光标位置不变
;	1---字符串中只含显示字符，其显示属性在BL中，显示后，光标位置改变
;	2---字符串中含显示字符和显示属性，显示后，光标位置不变
;	3---字符串中含显示字符和显示属性，显示后，光标位置改变
;无返回值

	mov sp, LOADER_BASE_ADDR			; 0x900-0x500为实模式下内存的可用区域
	mov bp, loadermsg				; ES:BP=字符串地址
	mov cx, 17					; CX=字符串长度
	mov ax, 0x1301					; AH=0x13, AL=0x01
	mov bx, 0x001f					; 页号为0，蓝底粉红字
	mov dx, 0x1800
	int 0x10

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
