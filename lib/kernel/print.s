TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0

[bits 32]
section .text
global put_char
put_char:
	pushad					; 备份32位寄存器环境
	mov ax, SELECTOR_VIDEO
	mov gs, ax

;获取当前光标位置
	mov dx, 0x03d4
	mov al, 0x0e
	out dx, al
	mov dx, 0x03d5
	in al, dx
	mov ah, al				; 高八位存于ah

	mov dx, 0x03d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x03d5
	in al, dx				; 低八位存于al
	mov bx, ax				; 光标位置存于bx

	mov ecx, [esp + 36]			; pushad压入32字节，返回地址占4字节，因此要打印的字符偏移36字节
	cmp cl, 0xd
	jz .is_carriage_return			; 回车
	cmp cl, 0xa
	jz .is_line_feed			; 换行
	cmp cl, 0x8
	jz .is_backspace			; 退格
	jmp .put_other				; 其他字符

.is_line_feed:					; \n \r 都表示为\n
.is_carriage_return:
	xor dx, dx
	mov ax, bx
	mov si, 80				; 每行80个字符
	div si
	sub bx, dx				; 减去余数后回到行首，回车结束
	add bx, 80				; 跳到下一行，换行结束
	cmp bx, 2000				; 判断是否超出页边界
	jl .set_cursor

.is_breakspace:
	dec bx
	shl bx, 1
	mov byte [gs:bx], 0x20
	inc bx
	mov byte [gs:bx], 0x07
	shr bx, 1
	jmp .set_cursor

.put_other:
	shl bx, 1
	mov byte [gs:bx], cl
	inc bx
	mov byte [gs:bx], 0x07
	shr bx, 1
	inc bx					; 指向下一个位置
	cmp bx, 2000
	jl .set_cursor

.roll_screen:
	cld
	mov ecx, 960
	lea esi, [gs:0x00]			; 第0行行首
	lea edi, [gs:0xa0]			; 第1行行首，注意每个字符占两字节(高字节为显示属性)
	rep movsd

	mov ebx, 3840				; 第24行行首
	mov ecx, 80
.cls:
	mov word [gs:ebx], 0x0720
	add ebx, 2
	loop .cls
	mov bx, 1920				; 最后一行行首索引

.set_cursor:
	mov dx, 0x03d4
	mov al, 0x0e
	out dx, al
	mov dx, 0x03d5
	mov al, bh
	out dx, al				; 设置高八位

	mov dx, 0x03d4
	mov al, 0x0f
	out dx, al
	mov dx 0x03d5
	mov al, bl
	out dx, al				; 设置低八位

.put_char_done:
	popad
	ret
