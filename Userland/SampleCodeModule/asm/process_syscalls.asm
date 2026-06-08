section .text

 GLOBAL sys_create
 GLOBAL sys_kill
 GLOBAL sys_getpid
 GLOBAL sys_yield
 GLOBAL sys_block
	 GLOBAL sys_nice
	 GLOBAL sys_ps
	 GLOBAL sys_unblock
	 GLOBAL sys_pipe_open
	 GLOBAL sys_pipe_close
	 GLOBAL sys_pipe_set_fd
	 GLOBAL sys_wait
	 GLOBAL sys_exit

 sys_create: 
    push rbp
    mov rbp, rsp
    mov r10, rcx 
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

sys_unblock: 
    push rbp
    mov rbp, rsp
    mov rax, 25
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_pipe_open:
    push rbp
    mov rbp, rsp
    mov rax, 30
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_pipe_close:
    push rbp
    mov rbp, rsp
    mov rax, 31
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_pipe_set_fd:
    push rbp
    mov rbp, rsp
    mov r10, rcx
    mov rax, 32
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_wait:
    push rbp
    mov rbp, rsp
    mov rax, 33
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_exit:
    push rbp
    mov rbp, rsp
    mov rax, 34
    int 0x80
    mov rsp, rbp
    pop rbp
    ret
