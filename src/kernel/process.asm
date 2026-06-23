; ==========================================
; 进程上下文切换 - process.asm
;
; 上下文切换的原理：
;   1. 保存当前进程的寄存器到它的栈上
;   2. 保存栈指针到当前进程的PCB
;   3. 从下一个进程的PCB加载栈指针
;   4. 从栈上恢复下一个进程的寄存器
;   5. 返回，实际上是跳转到下一个进程继续执行
;
; 为什么只保存 edi, esi, ebx, ebp 这几个寄存器？
;   因为根据C语言调用约定（cdecl）：
;   - eax, ecx, edx 是"调用者保存"的，调用者自己负责保存
;   - ebx, esi, edi, ebp 是"被调用者保存"的，被调用函数必须保存
;   所以我们作为被调用的函数，只需要保存这几个寄存器
;   加上返回地址eip，正好对应 registers_t 结构
; ==========================================

[bits 32]

; 导入C函数
extern process_tick

; ==========================================
; PCB字段偏移（必须和process.h里的结构体一致）
; ==========================================
; pid:            0  (4字节)
; name[32]:       4  (32字节)
; state:          36 (4字节)
; priority:       40 (4字节)
; time_slice:     44 (4字节)
; page_directory: 48 (4字节)
; kernel_stack:   52 (4字节)
; user_stack:     56 (4字节)
; context:        60 (4字节)  <- 重要！
; next:           64 (4字节)

PCB_CONTEXT_OFFSET      equ 60
PCB_PAGE_DIR_OFFSET     equ 48

; ==========================================
; 函数：context_switch
; 功能：上下文切换
; 参数（C调用约定，从右到左压栈）：
;   [esp + 8] = next  - 下一个进程的PCB
;   [esp + 4] = prev  - 当前进程的PCB（可以为NULL，表示第一次调度）
;   [esp]     = 返回地址
; ==========================================
global context_switch
context_switch:
    ; 建立栈帧
    push ebp
    mov  ebp, esp

    ; 保存被调用者保存寄存器
    ; 这些寄存器必须在函数返回前恢复原值
    push ebx
    push esi
    push edi

    ; ===== 保存当前进程的上下文 =====
    ; 获取prev指针（第一个参数，在ebp+8的位置）
    mov eax, [ebp + 8]   ; eax = prev

    ; 检查prev是否为NULL（第一次调度）
    test eax, eax
    jz .skip_save_context

    ; 保存当前栈指针到prev->context
    ; 现在栈上的内容（从低到高，栈顶到栈底）：
    ;   edi, esi, ebx, ebp, 返回地址
    ; 正好对应 registers_t 结构的顺序
    mov [eax + PCB_CONTEXT_OFFSET], esp  ; prev->context = esp

.skip_save_context:
    ; ===== 加载下一个进程的上下文 =====
    ; 获取next指针（第二个参数，在ebp+12的位置）
    mov eax, [ebp + 12]  ; eax = next

    ; 切换页目录（如果next有自己的页目录）
    mov ebx, [eax + PCB_PAGE_DIR_OFFSET]
    test ebx, ebx
    jz .skip_page_dir_switch

    ; 加载新的页目录到CR3
    mov cr3, ebx

.skip_page_dir_switch:
    ; 加载next的栈指针
    mov esp, [eax + PCB_CONTEXT_OFFSET]  ; esp = next->context

    ; 恢复寄存器（注意：顺序和push相反）
    pop edi
    pop esi
    pop ebx
    pop ebp

    ; 返回
    ; 注意：这时候栈顶是返回地址
    ; 对于新进程，这个返回地址就是它的入口函数
    ; 所以ret就会跳转到新进程开始执行
    ret

; ==========================================
; 函数：process_tick_asm
; 功能：时钟中断tick的包装函数
; 注意：这个函数会在时钟中断中被调用
;       用来更新进程时间片，触发调度
; ==========================================
global process_tick_asm
process_tick_asm:
    call process_tick
    ret
