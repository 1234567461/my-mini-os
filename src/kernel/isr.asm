; ==========================================
; ISR - 中断服务例程（汇编部分）
; ==========================================

[bits 32]
section .text

; ==========================================
; 宏定义：生成一个ISR入口
; 参数1：中断号
; 参数2：是否有错误码（1=有，0=没有）
; ==========================================

%macro ISR_NOERRCODE 1
    global isr%1
    isr%1:
        cli                     ; 关中断
        push byte 0             ; 压入一个假的错误码（占位，保持栈帧一致）
        push byte %1            ; 压入中断号
        jmp isr_common_stub     ; 跳转到公共处理部分
%endmacro

%macro ISR_ERRCODE 1
    global isr%1
    isr%1:
        cli                     ; 关中断
        ; 错误码已经由CPU自动压入了
        push byte %1            ; 压入中断号
        jmp isr_common_stub     ; 跳转到公共处理部分
%endmacro

; ==========================================
; 生成32个CPU异常ISR
; ==========================================

; 0-7：没有错误码
ISR_NOERRCODE 0   ; 除法错误
ISR_NOERRCODE 1   ; 调试异常
ISR_NOERRCODE 2   ; NMI中断
ISR_NOERRCODE 3   ; 断点
ISR_NOERRCODE 4   ; 溢出
ISR_NOERRCODE 5   ; 边界检查
ISR_NOERRCODE 6   ; 无效操作码
ISR_NOERRCODE 7   ; 设备不可用

; 8：有错误码（双重故障）
ISR_ERRCODE   8

; 9：没有错误码（协处理器段溢出）
ISR_NOERRCODE 9

; 10-14：有错误码
ISR_ERRCODE   10  ; 无效TSS
ISR_ERRCODE   11  ; 段不存在
ISR_ERRCODE   12  ; 栈段故障
ISR_ERRCODE   13  ; 通用保护故障
ISR_ERRCODE   14  ; 页故障

; 15-31：没有错误码（保留）
ISR_NOERRCODE 15
ISR_NOERRCODE 16  ; x87浮点异常
ISR_NOERRCODE 17  ; 对齐检查
ISR_NOERRCODE 18  ; 机器检查
ISR_NOERRCODE 19  ; SIMD浮点异常
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

; ==========================================
; 生成16个IRQ入口（重映射到32-47）
; ==========================================

%macro IRQ 2
    global irq%1
    irq%1:
        cli
        push byte 0             ; 假错误码
        push byte %2            ; 中断号（重映射后的）
        jmp irq_common_stub
%endmacro

IRQ 0,  32   ; 系统时钟
IRQ 1,  33   ; 键盘
IRQ 2,  34   ; 级联（从PIC）
IRQ 3,  35   ; 串口2
IRQ 4,  36   ; 串口1
IRQ 5,  37   ; 并口2
IRQ 6,  38   ; 软盘
IRQ 7,  39   ; 并口1
IRQ 8,  40   ; 实时时钟
IRQ 9,  41   ; 通用IO
IRQ 10, 42   ; 通用IO
IRQ 11, 43   ; 通用IO
IRQ 12, 44   ; PS/2鼠标
IRQ 13, 45   ; 协处理器
IRQ 14, 46   ; 主ATA硬盘
IRQ 15, 47   ; 从ATA硬盘

; ==========================================
; 公共ISR处理函数
; ==========================================

extern isr_handler   ; C语言的处理函数

isr_common_stub:
    ; 保存所有寄存器
    pusha           ; push eax, ecx, edx, ebx, esp, ebp, esi, edi

    mov ax, ds      ; 保存数据段
    push eax

    mov ax, 0x10    ; 加载内核数据段
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 调用C处理函数
    push esp        ; 把栈指针作为参数（指向寄存器结构体）
    call isr_handler
    add esp, 4      ; 清理参数

    ; 恢复数据段
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 恢复所有寄存器
    popa

    ; 清理栈上的中断号和错误码
    add esp, 8

    ; 中断返回
    sti             ; 开中断
    iret

; ==========================================
; 公共IRQ处理函数
; ==========================================

extern irq_handler   ; C语言的处理函数

irq_common_stub:
    ; 保存所有寄存器
    pusha

    mov ax, ds      ; 保存数据段
    push eax

    mov ax, 0x10    ; 加载内核数据段
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 调用C处理函数
    push esp
    call irq_handler
    add esp, 4

    ; 恢复数据段
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 恢复所有寄存器
    popa

    ; 清理栈上的中断号和错误码
    add esp, 8

    ; 中断返回
    sti
    iret

; ==========================================
; 加载IDT
; ==========================================

global idt_load
extern idt_ptr

idt_load:
    lidt [idt_ptr]
    ret
