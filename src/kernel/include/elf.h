/* ==========================================
 * ELF 文件格式头文件 - elf.h
 * 功能：
 *   1. ELF 文件结构定义
 *   2. ELF 加载器接口
 * ========================================== */
#ifndef ELF_H
#define ELF_H

#include "types.h"

/* ELF 魔数 */
#define ELF_MAGIC_0  0x7F
#define ELF_MAGIC_1  'E'
#define ELF_MAGIC_2  'L'
#define ELF_MAGIC_3  'F'

/* ELF 类型 */
#define ELF_TYPE_REL    1  /* 可重定位文件 */
#define ELF_TYPE_EXEC   2  /* 可执行文件 */
#define ELF_TYPE_DYN    3  /* 共享对象 */
#define ELF_TYPE_CORE   4  /* 核心转储 */

/* ELF 机器类型 */
#define ELF_MACHINE_386  3  /* Intel 80386 */

/* ELF 版本 */
#define ELF_VERSION_CURRENT  1

/* ELF 类 */
#define ELF_CLASS_32  1  /* 32位 ELF */
#define ELF_CLASS_64  2  /* 64位 ELF */

/* ELF 数据编码 */
#define ELF_DATA_LSB  1  /* 小端序 */
#define ELF_DATA_MSB  2  /* 大端序 */

/* 程序段类型 */
#define PT_NULL    0  /* 未使用 */
#define PT_LOAD    1  /* 可加载段 */
#define PT_DYNAMIC 2  /* 动态链接信息 */
#define PT_INTERP  3  /* 解释器路径 */
#define PT_NOTE    4  /* 备注 */
#define PT_SHLIB   5  /* 保留 */
#define PT_PHDR    6  /* 程序头表本身 */

/* 程序段标志 */
#define PF_X  0x1  /* 可执行 */
#define PF_W  0x2  /* 可写 */
#define PF_R  0x4  /* 可读 */

/* ==========================================
 * ELF 32位文件头
 * ========================================== */
typedef struct {
    uint8_t  e_ident[16];  /* 魔数和其他信息 */
    uint16_t e_type;       /* 文件类型 */
    uint16_t e_machine;    /* 机器类型 */
    uint32_t e_version;    /* 文件版本 */
    uint32_t e_entry;      /* 入口点地址 */
    uint32_t e_phoff;      /* 程序头表偏移 */
    uint32_t e_shoff;      /* 节头表偏移 */
    uint32_t e_flags;      /* 处理器特定标志 */
    uint16_t e_ehsize;     /* ELF 头大小 */
    uint16_t e_phentsize;  /* 程序头表条目大小 */
    uint16_t e_phnum;      /* 程序头表条目数 */
    uint16_t e_shentsize;  /* 节头表条目大小 */
    uint16_t e_shnum;      /* 节头表条目数 */
    uint16_t e_shstrndx;   /* 节名字符串表索引 */
} __attribute__((packed)) elf32_ehdr_t;

/* ==========================================
 * ELF 32位程序头
 * ========================================== */
typedef struct {
    uint32_t p_type;   /* 段类型 */
    uint32_t p_offset; /* 段在文件中的偏移 */
    uint32_t p_vaddr;  /* 段在内存中的虚拟地址 */
    uint32_t p_paddr;  /* 段在内存中的物理地址 */
    uint32_t p_filesz; /* 段在文件中的大小 */
    uint32_t p_memsz;  /* 段在内存中的大小 */
    uint32_t p_flags;  /* 段标志 */
    uint32_t p_align;  /* 段对齐 */
} __attribute__((packed)) elf32_phdr_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 验证 ELF 文件 */
int elf_validate(elf32_ehdr_t *ehdr);

/* 加载 ELF 可执行文件到当前进程地址空间 */
int elf_load_executable(void *elf_data, uint32_t *entry_point);

#endif /* ELF_H */
