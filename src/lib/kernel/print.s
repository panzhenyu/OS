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
	jmp .roll_screen

.is_backspace:
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
	mov esi, 0xc00b80a0			; 第1行行首，注意每个字符占两字节(高字节为显示属性)，不可写为 lea esi, [gs:0xa0]，原因不明
	mov edi, 0xc00b8000			; 第0行行首，注意每个字符占两字节(高字节为显示属性)
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
	mov dx, 0x03d5
	mov al, bl
	out dx, al				; 设置低八位

.put_char_done:
	popad
	ret

global put_str
put_str:
	push ebx
	push ecx
	xor ecx, ecx
	mov ebx, [esp+12]

.next_char:
	mov cl, [ebx]
	cmp cl, 0				; 遇到\0结束
	jz .put_str_done
	push ecx
	call put_char
	add esp, 4				; 恢复栈
	inc ebx
	jmp .next_char

.put_str_done:
	pop ecx
	pop ebx
	ret

global put_int32
put_int32:
	push eax				; eax存放要打印的32位整数
	push ebx
	push ecx				; ecx存放要打印的字符
	push edx
	push esi				; 存放位数

	mov eax, [esp+24]
	mov ebx, 10				; 除数
	xor esi, esi				; 个数初始化为0

	cmp eax, 0
	jg .get_num
	jz .get_zero
	mov ecx, '-'				; 负数则先打印负号
	push ecx
	call put_char
	add esp, 4
	neg eax					; 负数转正数

.get_num:
	cmp eax, 0
	jz .put_num
	xor edx, edx				; 32位除法每次需将edx置0
	idiv ebx
	mov ecx, edx
	add ecx, 0x30				; 转为ascii码
	push ecx				; 压栈
	inc esi					; 记录个数
	jmp .get_num

.get_zero:
	mov ecx, '0'
	push ecx
	inc esi

.put_num:
	cmp esi, 0
	jz .put_int_done
	call put_char
	add esp, 4
	dec esi
	jmp .put_num

.put_int_done:
	pop esi
	pop edx
	pop ecx
	pop ebx
	pop eax
	ret

global put_uint32_hex
put_uint32_hex:
	ret
