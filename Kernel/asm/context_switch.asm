GLOBAL contextSwitch

SECTION .text

contextSwitch:
    mov [rdi], rsp ; save current process's RSP into its PCB
    mov rsp, rsi ; load next process's RSP from its PCB
    ret ; return into the next process