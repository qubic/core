PUBLIC __customStackSetupAndRunFunc

.code

__customStackSetupAndRunFunc PROC
    ; arguments are passed in registers:
    ; - RCX   newStackTop
    ; - RDX   funcToCall
    ; - R8    dataToPass
    ; (x64 UEFI/Windows calling convention https://uefi.org/specs/UEFI/2.9_A/02_Overview.html?highlight=stack#detailed-calling-conventions)

    ; store rbp of caller
    push rbp

    ; replace stack and store old rsp on stack
    mov rbp, rsp                ; save rsp in rbp for later pushing it onto new stack
    mov rsp, rcx                ; use newStackTop as function call stack stack
    push rbp                    ; push rbp (old stack pointer) to stack

    ; prepare stack for call
    mov rbp, rsp                ; save rsp from before function call in rbp
    sub rsp, 32                 ; add shadow space required for function call
    and spl, -16                ; align stack at 16

    ; set parameter and call function
    mov rcx, r8                 ; move r8 (data to pass) to rcx (first param of call)
    call rdx                    ; call funcToCall function pointer

    ; restore rbp and rsp of caller and return
    mov rsp, rbp                ; restore rsp from before function call
    pop rsp                     ; get old stack pointer from stack (switching back to original stack)
    pop rbp                     ; restore rbp of caller
    ret
__customStackSetupAndRunFunc ENDP

END
