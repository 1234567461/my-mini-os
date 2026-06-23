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
; 内存操作（快速版本，用汇编优化）
; ==========================================

; void memset(void *dst, uint8_t val, size_t count)
global memset
memset:
    push edi
    mov edi, [esp + 8]   ; dst
    mov eax, [esp + 12]  ; val
    mov ecx, [esp + 16]  ; count
    rep stosb
    pop edi
    ret

; void memcpy(void *dst, const void *src, size_t count)
global memcpy
memcpy:
    push esi
    push edi
    mov edi, [esp + 12]  ; dst
    mov esi, [esp + 16]  ; src
    mov ecx, [esp + 20]  ; count
    rep movsb
    pop edi
    pop esi
    ret

; int memcmp(const void *s1, const void *s2, size_t count)
global memcmp
memcmp:
    push esi
    push edi
    mov esi, [esp + 12]  ; s1
    mov edi, [esp + 16]  ; s2
    mov ecx, [esp + 20]  ; count
    repe cmpsb
    jz .equal
    mov eax, 1
    jmp .done
.equal:
    mov eax, 0
.done:
    pop edi
    pop esi
    ret
