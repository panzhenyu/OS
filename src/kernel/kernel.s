%define ZERO push 0
%define ERROR_CODE nop

extern put_str;
extern idt_table;

section .data
intr_str db "interrupt occcur!", 0xa, 0

global intr_entry_table
intr_entry_table:

%macro VECTOR 2

section .text
intr%1entry:
	%2
	push ds
	push es
	push fs
	push gs
	pushad

	mov al, 0x20			; 为什么不放在call后面?
	out 0xa0, al
	out 0x20, al

	push %1				; 通用的中断处理程序需要中断号作为参数
	call [idt_table + %1 * 4]
	jmp intr_exit

section .data
	dd intr%1entry

%endmacro

section .text
global intr_exit
intr_exit:
	add esp, 4			; 跳过中断号
	popad
	pop gs
	pop fs
	pop es
	pop ds
	add esp, 4			; 跳过错误码
	iretd

VECTOR 0x00, ZERO
VECTOR 0x01, ZERO
VECTOR 0x02, ZERO
VECTOR 0x03, ZERO
VECTOR 0x04, ZERO
VECTOR 0x05, ZERO
VECTOR 0x06, ZERO
VECTOR 0x07, ZERO
VECTOR 0x08, ERROR_CODE
VECTOR 0x09, ZERO
VECTOR 0x0a, ERROR_CODE
VECTOR 0x0b, ERROR_CODE
VECTOR 0x0c, ERROR_CODE
VECTOR 0x0d, ERROR_CODE
VECTOR 0x0e, ERROR_CODE
VECTOR 0x0f, ZERO

VECTOR 0x10, ZERO
VECTOR 0x11, ERROR_CODE
VECTOR 0x12, ZERO
VECTOR 0x13, ZERO
VECTOR 0x14, ZERO
VECTOR 0x15, ZERO
VECTOR 0x16, ZERO
VECTOR 0x17, ZERO
VECTOR 0x18, ZERO
VECTOR 0x19, ZERO
VECTOR 0x1a, ZERO
VECTOR 0x1b, ZERO
VECTOR 0x1c, ZERO
VECTOR 0x1d, ZERO
VECTOR 0x1e, ERROR_CODE
VECTOR 0x1f, ZERO

VECTOR 0x20, ZERO			; 时钟中断
VECTOR 0x21, ZERO			; 键盘中断
VECTOR 0x22, ZERO			; 8295a级联
VECTOR 0x23, ZERO			; 串口2
VECTOR 0x24, ZERO			; 串口1
VECTOR 0x25, ZERO			; 并口2
VECTOR 0x26, ZERO			; 软盘
VECTOR 0x27, ZERO			; 并口1
VECTOR 0x28, ZERO			; 实时时钟
VECTOR 0x29, ZERO			; 重定向
VECTOR 0x2a, ZERO			; 保留
VECTOR 0x2b, ZERO			; 保留
VECTOR 0x2c, ZERO			; ps/2鼠标
VECTOR 0x2d, ZERO			; fpu浮点异常
VECTOR 0x2e, ZERO			; 硬盘
VECTOR 0x2f, ZERO			; 保留

; 0x80号中断
[bits 32]
extern syscall_table
section .text
global syscall_handler
syscall_handler:
	push 0
	push ds
	push es
	push fs
	push gs
	pushad

	push 0x80				; 系统调用号
	push edx				; 系统调用第三个参数
	push ecx				; 系统调用第二个参数
	push ebx				; 系统调用第一个参数
	call [syscall_table + eax*4]
	add esp, 12				; 跨过三个传入参数
	mov [esp + 8*4], eax	; 由于中断返回后会恢复寄存器，因此eax必须保存在中断栈的eax处

	jmp intr_exit