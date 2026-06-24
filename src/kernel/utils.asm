; ==========================================
; 底层汇编工具函数
; ==========================================

[bits 32]
section .text

; ==========================================
; 端口读写函数
; ==========================================

; void outb(uint16_t port, uint8_t value)
global outb
outb:
    mov dx, [esp + 4]   ; port
    mov al, [esp + 8]   ; value
    out dx, al
    ret

; uint8_t inb(uint16_t port)
global inb
inb:
    mov dx, [esp + 4]   ; port
    in al, dx
    ret

; void outw(uint16_t port, uint16_t value)
global outw
outw:
    mov dx, [esp + 4]   ; port
    mov ax, [esp + 8]   ; value
    out dx, ax
    ret

; uint16_t inw(uint16_t port)
global inw
inw:
    mov dx, [esp + 4]   ; port
    in ax, dx
    ret

; void outl(uint16_t port, uint32_t value)
global outl
outl:
    mov dx, [esp + 4]   ; port
    mov eax, [esp + 8]  ; value
    out dx, eax
    ret

; uint32_t inl(uint16_t port)
global inl
inl:
    mov dx, [esp + 4]   ; port
    in eax, dx
    ret

; ==========================================
; 中断控制
; ==========================================

; void cli() - 关中断
global cli
cli:
    cli
    ret

; void sti() - 开中断
global sti
sti:
    sti
    ret

; void hlt() - 停机
global hlt
hlt:
    hlt
    ret

; ==========================================
; GDT和TSS操作
; ==========================================

; void gdt_flush(uint32_t gdt_ptr)
global gdt_flush
gdt_flush:
    lgdt [esp + 4]
    ret

; void tss_flush(uint16_t sel)
global tss_flush
tss_flush:
    mov ax, [esp + 4]
    ltr ax
    ret

; ==========================================
; 内存操作（快速版本，用汇编优化）
; 注意：memset/memcpy/memcmp 已由 string.c 提供
; ==========================================
