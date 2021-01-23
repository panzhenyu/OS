[bits 32]
global switch_to
switch_to:
    push esi
    push edi
    push ebx
    push ebp
    mov eax, [esp + 20]         ; cur指针
    mov [eax], esp
    
    mov eax, [esp + 24]
    mov esp, [eax]
    pop ebp
    pop ebx
    pop edi
    pop esi
    ret
