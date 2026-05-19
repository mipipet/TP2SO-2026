section .text

 GLOBAL sys_create
 GLOBAL sys_kill
 GLOBAL sys_getpid
 GLOBAL sys_yield
 GLOBAL sys_block
 GLOBAL sys_nice
 GLOBAL sys_ps

 sys_create: 
    push rbp
    mov rbp, rsp
    mov rax, 18
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

 sys_kill: 
    push rbp
    mov rbp, rsp
    mov rax, 19
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

 sys_getpid: 
    push rbp
    mov rbp, rsp
    mov rax, 20
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_yield: 
    push rbp
    mov rbp, rsp
    mov rax, 21
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_block: 
    push rbp
    mov rbp, rsp
    mov rax, 22
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_nice: 
    push rbp
    mov rbp, rsp
    mov rax, 23
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_ps: 
    push rbp
    mov rbp, rsp
    mov rax, 24
    int 0x80
    mov rsp, rbp
    pop rbp
    ret