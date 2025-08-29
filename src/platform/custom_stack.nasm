global __customStackSetupAndRunFunc

section .text

; arguments in registers:
; RCX = newStackTop
; RDX = funcToCall
; R8  = dataToPass

__customStackSetupAndRunFunc:
    ; store caller's RBP
    push rbp

    ; switch to new stack and save old RSP
    mov rbp, rsp        ; save old rsp in rbp
    mov rsp, rcx        ; set rsp to newStackTop
    push rbp            ; save old stack pointer on new stack

    ; prepare stack for function call
    mov rbp, rsp
    sub rsp, 32         ; shadow space
    and rsp, -16        ; align stack to 16 bytes

    ; move parameter and call function
    mov rcx, r8         ; first argument
    call rdx            ; call function pointer

    ; restore stack
    mov rsp, rbp        ; restore rsp from before call
    pop rsp             ; restore old stack pointer
    pop rbp             ; restore ca
