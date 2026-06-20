; ==========================================
; 第一阶段引导扇区 - Boot Sector
; 功能：加载第二阶段loader到内存，然后跳转
; ==========================================

org 0x7c00

; ====== 初始化寄存器 ======
mov ax, 0
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7c00

; ====== 清屏 ======
mov ah, 0x00
mov al, 0x03
int 0x10

; ====== 显示启动信息 ======
mov si, boot_msg
call print_string

; ====== 加载第二阶段Loader ======
; BIOS 读磁盘功能: int 0x13
; ah = 0x02 (读扇区)
; al = 扇区数
; ch = 柱面号
; cl = 扇区号 (从1开始)
; dh = 磁头号
; dl = 驱动器号
; es:bx = 内存缓冲区地址

mov bx, 0x1000      ; 把loader加载到 0x1000:0x0000 = 0x10000
mov es, bx
mov bx, 0

mov ah, 0x02        ; 功能号：读扇区
mov al, 4           ; 读4个扇区 (2KB，足够放loader了)
mov ch, 0           ; 柱面0
mov cl, 2           ; 扇区2 (第1个扇区是bootsect自己)
mov dh, 0           ; 磁头0
mov dl, 0x80        ; 驱动器号 (0x80 = 第一个硬盘)

int 0x13            ; 调用BIOS磁盘中断
jc disk_error       ; 如果出错，CF标志位会被置1

; ====== 跳转到第二阶段Loader ======
mov si, jump_msg
call print_string

jmp 0x1000:0x0000   ; 远跳转，跳到loader

; ==========================================
; 错误处理
; ==========================================

disk_error:
    mov si, disk_err_msg
    call print_string
    jmp $

; ==========================================
; 函数：print_string
; 打印以0结尾的字符串
; 输入：si = 字符串地址
; ==========================================
print_string:
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
; 数据区
; ==========================================

boot_msg     db '[Boot] My Mini OS is starting...', 0x0d, 0x0a, 0
disk_err_msg db '[Error] Disk read failed!', 0x0d, 0x0a, 0
jump_msg     db '[Boot] Jumping to loader...', 0x0d, 0x0a, 0x0d, 0x0a, 0

; ==========================================
; 填充到512字节，引导扇区魔数
; ==========================================

times 510 - ($ - $$) db 0
dw 0xaa55
