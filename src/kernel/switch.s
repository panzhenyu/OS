[bits 32]
global switch_to
switch_to:
    push esi
    push edi
    push ebx
    push ebp
    mov eax, [esp + 20]
    