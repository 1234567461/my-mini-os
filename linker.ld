/* ==========================================
 * 内核链接脚本
 * ========================================== */

ENTRY(kernel_entry)   /* 入口点 */

SECTIONS
{
    . = 0x20000;      /* 内核加载地址（和loader一致） */

    .text ALIGN(4K) : {
        *(.text)       /* 代码段 */
    }

    .rodata ALIGN(4K) : {
        *(.rodata*)    /* 只读数据段 */
    }

    .data ALIGN(4K) : {
        *(.data)       /* 数据段 */
    }

    .bss ALIGN(4K) : {
        __bss_start = .;
        *(COMMON)
        *(.bss)        /* BSS段（未初始化数据） */
        __bss_end = .;
    }

    /* 内核结束地址（用于内存管理） */
    kernel_end = .;
}
