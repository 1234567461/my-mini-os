; ==========================================
; 第二阶段加载器 - Loader
; 功能：
;   1. 显示加载信息
;   2. 设置GDT（全局描述符表）
;   3. 进入32位保护模式
;   4. 跳转到C语言内核
; ==========================================

org 0x10000          ; 我们被加载到 0x10000 的位置

[bits 16]

; ====== 显示加载信息 ======
mov si, loader_msg
call print_string_16

; ====== 加载内核到内存 ======
mov si, load_kernel_msg
call print_string_16

; 内核在第6个扇区开始（bootsect占1个，loader占4个，所以从第6个开始）
; 把内核加载到 0x2000:0x0000 = 0x20000
mov bx, 0x2000
mov es, bx
mov bx, 0x0000

mov dh, 0           ; 磁头号
mov dl, 0x80        ; 驱动器号
mov ch, 0           ; 柱面号
mov cl, 6           ; 起始扇区号（第6个扇区）
mov al, 100         ; 读100个扇区（50KB，足够放内核了）

call disk_load

; ====== 切换到保护模式 ======
mov si, switch_pm_msg
call print_string_16

cli                 ; 关中断
lgdt [gdt_descriptor]  ; 加载GDT

; 设置CR0寄存器的第0位（PE位），进入保护模式
mov eax, cr0
or eax, 0x1
mov cr0, eax

; 远跳转：刷新流水线，正式进入32位模式
jmp CODE_SEG:init_pm

; ==========================================
; 函数：print_string_16
; 16位实模式下打印字符串
; ==========================================
print_string_16:
    mov ah, 0x0e
    mov bh, 0x00
    mov bl, 0x0f
.repeat:
    lodsb
    or al, al
    jz .done
    int 0x10
    jmp .repeat
.done:
    ret

; ==========================================
; 函数：disk_load
; 读取磁盘扇区
; ==========================================
disk_load:
    pusha
    push ax

    mov ah, 0x02
    int 0x13

    jc disk_error

    pop dx
    cmp al, dl
    jne disk_error

    popa
    ret

disk_error:
    mov si, disk_err_msg
    call print_string_16
    jmp $

; ==========================================
; GDT - 全局描述符表
; ==========================================

gdt_start:

; 空描述符（必须有）
gdt_null:
    dd 0x0
    dd 0x0

; 代码段描述符
gdt_code:
    dw 0xffff       ; 段界限（0-15）
    dw 0x0          ; 基地址（0-15）
    db 0x0          ; 基地址（16-23）
    db 10011010b    ; 标志：存在、特权级0、代码段、可读
    db 11001111b    ; 标志：4KB粒度、32位、段界限（16-19）
    db 0x0          ; 基地址（24-31）

; 数据段描述符
gdt_data:
    dw 0xffff       ; 段界限
    dw 0x0          ; 基地址
    db 0x0          ; 基地址
    db 10010010b    ; 标志：存在、特权级0、数据段、可写
    db 11001111b    ; 标志：4KB粒度、32位、段界限
    db 0x0          ; 基地址

gdt_end:

; GDT描述符
gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; GDT大小
    dd gdt_start                ; GDT基地址

; 段选择子常量
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; ==========================================
; 32位保护模式初始化
; ==========================================

[bits 32]

init_pm:
    ; 设置所有数据段寄存器
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 设置栈
    mov ebp, 0x90000
    mov esp, ebp

    ; 跳转到C语言内核（内核在0x20000）
    call KERNEL_OFFSET

    ; 如果内核返回了（不应该发生）
    jmp $

; ==========================================
; 常量定义
; ==========================================
KERNEL_OFFSET equ 0x20000

; ==========================================
; 数据区
; ==========================================

loader_msg      db '[Loader] Second stage loader loaded!', 0x0d, 0x0a, 0
load_kernel_msg db '[Loader] Loading kernel...', 0x0d, 0x0a, 0
switch_pm_msg   db '[Loader] Switching to 32-bit protected mode...', 0x0d, 0x0a, 0x0d, 0x0a, 0
disk_err_msg    db '[Error] Disk read failed!', 0x0d, 0x0a, 0
