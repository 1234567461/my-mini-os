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

; ====== 尝试设置VBE/VESA图形模式 ======
; 目标模式：640x480x32 (模式号 0x118 或自动检测)
mov si, vbe_try_msg
call print_string_16

call detect_vbe_mode
cmp ax, 0
jne vbe_success

; VBE失败，使用VGA文本模式
mov si, vbe_fail_msg
call print_string_16
jmp vbe_done

vbe_success:
    mov si, vbe_ok_msg
    call print_string_16

vbe_done:

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

; ====== 将VBE信息复制到约定地址 0x7000 ======
; 内核会从这里读取VBE信息
mov ax, cs
mov ds, ax

mov si, vbe_available
mov di, 0x7000
mov cx, 12         ; VBE信息大小: 1+2+2+1+2+4 = 12字节
cld
rep movsb

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
; 函数：detect_vbe_mode
; 功能：检测并设置VBE/VESA图形模式
; 返回：AX=0 失败, AX=1 成功
; 使用模式：640x480x32 (模式0x118)
; VBE信息存储在 vbe_info 结构体中
; ==========================================
detect_vbe_mode:
    pusha

    ; 第一步：获取VBE控制器信息
    mov ax, 0x4F00
    mov di, vbe_info_block
    int 0x10
    cmp ax, 0x004F
    jne .vbe_not_supported

    ; 第二步：获取模式信息（尝试640x480x32 = 模式0x118）
    mov ax, 0x4F01
    mov cx, 0x118          ; 模式号：640x480x32
    mov di, mode_info_block
    int 0x10
    cmp ax, 0x004F
    jne .try_other_modes

    ; 检查模式属性：是否支持线性帧缓冲
    mov ax, [mode_info_block + 0x00]  ; ModeAttributes
    test ax, 0x0100                    ; 线性帧缓冲支持
    jz .try_other_modes

    ; 获取分辨率和bpp
    mov ax, [mode_info_block + 0x12]  ; XResolution
    mov [vbe_width], ax
    mov ax, [mode_info_block + 0x14]  ; YResolution
    mov [vbe_height], ax
    mov al, [mode_info_block + 0x19]  ; BitsPerPixel
    mov [vbe_bpp], al
    mov eax, [mode_info_block + 0x28] ; PhysBasePtr (线性帧缓冲地址)
    mov [vbe_phys_addr], eax
    mov ax, [mode_info_block + 0x10]  ; BytesPerScanLine
    mov [vbe_pitch], ax

    ; 第三步：设置图形模式
    mov ax, 0x4F02
    mov bx, 0x4118       ; 模式号 | 0x4000 (使用线性帧缓冲)
    int 0x10
    cmp ax, 0x004F
    jne .try_other_modes

    ; 成功
    mov byte [vbe_available], 1
    popa
    mov ax, 1
    ret

.try_other_modes:
    ; 尝试 640x480x24 (模式0x117)
    mov ax, 0x4F01
    mov cx, 0x117
    mov di, mode_info_block
    int 0x10
    cmp ax, 0x004F
    jne .try_640x480x16

    mov ax, [mode_info_block + 0x00]
    test ax, 0x0100
    jz .try_640x480x16

    mov ax, [mode_info_block + 0x12]
    mov [vbe_width], ax
    mov ax, [mode_info_block + 0x14]
    mov [vbe_height], ax
    mov al, [mode_info_block + 0x19]
    mov [vbe_bpp], al
    mov eax, [mode_info_block + 0x28]
    mov [vbe_phys_addr], eax
    mov ax, [mode_info_block + 0x10]
    mov [vbe_pitch], ax

    mov ax, 0x4F02
    mov bx, 0x4117
    int 0x10
    cmp ax, 0x004F
    jne .try_640x480x16

    mov byte [vbe_available], 1
    popa
    mov ax, 1
    ret

.try_640x480x16:
    ; 尝试 640x480x16 (模式0x114)
    mov ax, 0x4F01
    mov cx, 0x114
    mov di, mode_info_block
    int 0x10
    cmp ax, 0x004F
    jne .vbe_not_supported

    mov ax, [mode_info_block + 0x00]
    test ax, 0x0100
    jz .vbe_not_supported

    mov ax, [mode_info_block + 0x12]
    mov [vbe_width], ax
    mov ax, [mode_info_block + 0x14]
    mov [vbe_height], ax
    mov al, [mode_info_block + 0x19]
    mov [vbe_bpp], al
    mov eax, [mode_info_block + 0x28]
    mov [vbe_phys_addr], eax
    mov ax, [mode_info_block + 0x10]
    mov [vbe_pitch], ax

    mov ax, 0x4F02
    mov bx, 0x4114
    int 0x10
    cmp ax, 0x004F
    jne .vbe_not_supported

    mov byte [vbe_available], 1
    popa
    mov ax, 1
    ret

.vbe_not_supported:
    mov byte [vbe_available], 0
    popa
    xor ax, ax
    ret

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
vbe_try_msg     db '[Loader] Detecting VBE/VESA graphics...', 0x0d, 0x0a, 0
vbe_ok_msg      db '[Loader] VBE mode set: 640x480', 0x0d, 0x0a, 0
vbe_fail_msg    db '[Loader] VBE not available, using VGA text mode', 0x0d, 0x0a, 0

; ==========================================
; VBE信息缓冲区（与C内核共享）
; 地址：0x8000 开始，确保在实模式可访问范围内
; ==========================================
vbe_info_block      equ 0x8000   ; VBE控制器信息块 (512字节)
mode_info_block     equ 0x8200   ; 模式信息块 (256字节)

; VBE信息全局变量（供内核读取）
vbe_available   db 0            ; 1=VBE可用, 0=不可用
vbe_width       dw 640          ; 水平分辨率
vbe_height      dw 480          ; 垂直分辨率
vbe_bpp         db 32           ; 每像素位数
vbe_pitch       dw 2560         ; 每行字节数 (640*4)
vbe_phys_addr   dd 0xE0000000   ; 线性帧缓冲物理地址
