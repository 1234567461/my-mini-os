/* ==========================================
 * 内核链接脚本 - linker.ld
 * 功能：告诉链接器如何组织内核的各个段
 * ========================================== */

ENTRY(kernel_entry)   /* 入口点：kernel_entry.asm 里的标签 */

SECTIONS
{
    . = 0x20000;      /* 内核加载地址：0x20000 */

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
}
