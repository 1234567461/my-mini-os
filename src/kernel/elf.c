/* ==========================================
 * ELF 文件加载器实现 - elf.c
 * 功能：
 *   1. ELF 文件验证
 *   2. ELF 可执行文件加载
 * ========================================== */
#include "elf.h"
#include "string.h"
#include "vga.h"
#include "paging.h"
#include "memory.h"
#include "heap.h"
#include "task.h"

/* ==========================================
 * 函数：elf_validate
 * 功能：验证 ELF 文件是否有效
 * 返回：0=有效，-1=无效
 * ========================================== */
int elf_validate(elf32_ehdr_t *ehdr)
{
    if (ehdr == NULL) {
        return -1;
    }
    
    /* 检查魔数 */
    if (ehdr->e_ident[0] != ELF_MAGIC_0 ||
        ehdr->e_ident[1] != ELF_MAGIC_1 ||
        ehdr->e_ident[2] != ELF_MAGIC_2 ||
        ehdr->e_ident[3] != ELF_MAGIC_3) {
        return -1;
    }
    
    /* 检查是否为 32 位 ELF */
    if (ehdr->e_ident[4] != ELF_CLASS_32) {
        return -1;
    }
    
    /* 检查是否为小端序 */
    if (ehdr->e_ident[5] != ELF_DATA_LSB) {
        return -1;
    }
    
    /* 检查版本 */
    if (ehdr->e_version != ELF_VERSION_CURRENT) {
        return -1;
    }
    
    /* 检查是否为可执行文件 */
    if (ehdr->e_type != ELF_TYPE_EXEC) {
        return -1;
    }
    
    /* 检查是否为 386 架构 */
    if (ehdr->e_machine != ELF_MACHINE_386) {
        return -1;
    }
    
    return 0;
}

/* ==========================================
 * 函数：elf_load_executable
 * 功能：加载 ELF 可执行文件到当前进程地址空间
 * 参数：
 *   elf_data - ELF 文件数据指针
 *   entry_point - 返回入口点地址
 * 返回：0=成功，-1=失败
 * ========================================== */
int elf_load_executable(void *elf_data, uint32_t *entry_point)
{
    if (elf_data == NULL || entry_point == NULL) {
        return -1;
    }
    
    elf32_ehdr_t *ehdr = (elf32_ehdr_t *)elf_data;
    
    /* 验证 ELF 文件 */
    if (elf_validate(ehdr) != 0) {
        return -1;
    }
    
    /* 获取程序头表 */
    elf32_phdr_t *phdrs = (elf32_phdr_t *)((uint8_t *)elf_data + ehdr->e_phoff);
    
    /* 遍历所有程序段 */
    for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
        elf32_phdr_t *phdr = &phdrs[i];
        
        /* 只处理可加载段 */
        if (phdr->p_type != PT_LOAD) {
            continue;
        }
        
        /* 简化版：我们假设程序段已经在内核地址空间中
         * 实际上需要映射到用户空间
         * 这里我们只复制数据到目标地址
         */
        
        uint8_t *dest = (uint8_t *)phdr->p_vaddr;
        uint8_t *src = (uint8_t *)elf_data + phdr->p_offset;
        
        /* 复制文件中的数据 */
        if (phdr->p_filesz > 0) {
            memcpy(dest, src, phdr->p_filesz);
        }
        
        /* 清零剩余部分（bss 段等） */
        if (phdr->p_memsz > phdr->p_filesz) {
            memset(dest + phdr->p_filesz, 0, phdr->p_memsz - phdr->p_filesz);
        }
    }
    
    /* 返回入口点 */
    *entry_point = ehdr->e_entry;
    
    return 0;
}
