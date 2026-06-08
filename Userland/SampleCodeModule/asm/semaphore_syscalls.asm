section .text

GLOBAL sys_sem_open
GLOBAL sys_sem_wait
GLOBAL sys_sem_post
GLOBAL sys_sem_close

sys_sem_open:
    push rbp
    mov rbp, rsp

    mov rax, 26
    int 0x80
    
    mov rsp, rbp
    pop rbp
    ret

sys_sem_wait:
    push rbp
    mov rbp, rsp

    mov rax, 27
    int 0x80

    mov rsp, rbp
    pop rbp
    ret

sys_sem_post:
    push rbp
    mov rbp, rsp

    mov rax, 28
    int 0x80

    mov rsp, rbp
    pop rbp
    ret

sys_sem_close:
    push rbp
    mov rbp, rsp

    mov rax, 29
    int 0x80

    mov rsp, rbp
    pop rbp
    ret
