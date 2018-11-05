;主引导程序
SECTION MBR vstart=0x7c00
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov fs, ax
	mov sp, 0x7c00

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
	mov cx, 0		; 左上角
	mov dx, 0x184f		; 右下角(80,25)
	int 0x10

;获取光标位置
;.get_cursor获取当前光标位置，在光标位置处打印字符
	mov ah, 3		; 3号功能获取光标位置，需要存入ah寄存器
	mov bh, 0		; bh寄存器存储带获取的光标的页号
	int 0x10		; 输出:
				; CH=光标开始行，CL=光标结束行，DH=光标所在行，DL=光标所在列

;打印字符串
	mov ax, message
	mov bp, ax		; es:bp为串首地址，es此时与cs一致
	; 光标位置要用到ds寄存器中的内容，cs中的光标可忽略
	mov cx, 5		; cx位串长度，不包括结束符0
	mov ax, 0x1301		; AH=13是显示字符及属性, AL设置写字符方式 AL=01:显示字符串，光标跟随移动
	mov bx, 0x0002		; BH存储要显示的页号，此处是第零页, BL中是字符属性 BL=02h:黑底绿字
	int 0x10

	jmp $			; 使程序悬停在此

	message db "1 MBR"	; q:似乎只能显示五个字符
	times 510-($-$$) db 0
	db 0x55, 0xaa
