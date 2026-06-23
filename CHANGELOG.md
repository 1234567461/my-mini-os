# Changelog

All notable changes to this project will be documented in this file.

## [v0.5.0] - 2026-06-23

### 🎉 Overview
v0.5.0 is a major release that introduces memory isolation, user permissions, virtual file system, and disk support. This version lays the foundation for a more secure and capable operating system.

### ✨ New Features

#### 🔐 Memory Management & Protection
- **Extended physical memory**: 32MB → 128MB (32768 pages)
- **Large page support**: 4MB large pages (PSE) for kernel space, reducing TLB misses
- **Full virtual memory**: Complete two-level page table implementation
- **Kernel/User space isolation**: 
  - Kernel space: 0x00000000 - 0x003FFFFF (4MB, Ring 0 exclusive)
  - User space: 0x00400000 - 0x07FFFFFF (124MB, Ring 3 accessible)
- **Independent address spaces**: Each process has its own page directory (CR3 switching)
- **Page fault handling**: Captures illegal memory access with detailed error info
- **Independent stacks**: Separate kernel stack and user stack for each process

#### 👤 User Permission System
- **4 user levels**: Guest, Normal User, Administrator, Kernel
- **`su` command**: Switch users with password authentication
- **`whoami` command**: View current username and permission level
- **Privilege checking**: Restricted commands (dmesg, reboot) require admin rights
- **Default login**: Starts as normal user "user", use `su root` to switch to admin
- **Built-in users**:
  - root / toor - Administrator
  - user / 123456 - Normal user
  - guest / (empty) - Guest
  - kernel / (empty) - Kernel user

#### 📁 Virtual File System (VFS)
- **VFS abstraction layer**: Unified interface for multiple file systems
- **File system operations**: open, close, read, write, create, unlink, mkdir, rmdir, readdir, stat
- **File types**: Regular files and directories
- **Path resolution**: Relative and absolute path support

#### 💾 RAM File System (ramfs)
- **In-memory file system**: Tree-structured directory hierarchy
- **Full file operations**: create, read, write, delete
- **Directory support**: mkdir, rmdir, cd, pwd
- **Shell commands**: ls, cat, mkdir, rm, cd, pwd, touch

#### 💿 Disk & Block Device Layer
- **Block device abstraction**: Unified block device interface
- **Buffer cache**: 64-block LRU cache for improved disk I/O performance
- **Dirty buffer tracking**: Write-back caching with block_sync()
- **ATA/IDE disk driver**: PIO mode support
  - Read/write sectors
  - Drive identification
  - Primary channel support

#### 📂 FAT16 File System
- **Complete FAT16 support**:
  - FAT table operations (read, write, allocate clusters)
  - Directory entry management
  - 8.3 filename conversion
- **File operations**:
  - Create, read, write, delete files
  - Multi-cluster file support
  - File size tracking
- **Root directory support**: Full file operations in root directory
- **VFS compatible**: Implements all VFS operations

#### 🔧 System Enhancements
- **System reboot**: `reboot` command via 8042 keyboard controller
- **Kernel log**: `dmesg` command with ring buffer
  - Boot logging for all subsystems
  - Multiple log levels
- **System call framework**: int 0x80 soft interrupt
  - Basic syscalls: exit, write, read, open, close, getpid, brk
  - Parameter passing via registers

### 📊 Architecture Improvements
- 32-bit x86 Protected Mode
- Separate kernel and user address spaces
- Memory protection via page permissions
- Modular driver architecture
- Layered file system design (VFS → FS → Block Device)

### 🧪 Shell Commands
**New commands added in v0.5.0:**
- `ls` - List directory contents
- `cat` - Display file contents
- `mkdir` - Create directory
- `rm` - Remove file
- `cd` - Change directory
- `pwd` - Print working directory
- `touch` - Create empty file
- `su` - Switch user
- `whoami` - Show current user
- `dmesg` - Show kernel boot log
- `reboot` - Reboot system

### 📁 Project Structure
```
my-mini-os/
├── src/
│   ├── boot/
│   │   ├── bootsect.asm      # Boot sector (512 bytes)
│   │   └── loader.asm        # Second stage loader
│   └── kernel/
│       ├── include/          # Header files
│       │   ├── types.h       # Basic types
│       │   ├── string.h      # String functions
│       │   ├── vga.h         # VGA display
│       │   ├── idt.h         # Interrupt Descriptor Table
│       │   ├── isr.h         # Interrupt Service Routines
│       │   ├── pic.h         # Programmable Interrupt Controller
│       │   ├── pit.h         # Programmable Interval Timer
│       │   ├── keyboard.h    # PS/2 keyboard
│       │   ├── memory.h      # Physical memory manager
│       │   ├── paging.h      # Paging mechanism
│       │   ├── heap.h        # Heap allocator
│       │   ├── task.h        # Task/process management
│       │   ├── shell.h       # Shell command line
│       │   ├── klog.h        # Kernel logging
│       │   ├── system.h      # System functions
│       │   ├── user.h        # User permissions
│       │   ├── vfs.h         # Virtual File System
│       │   ├── ramfs.h       # RAM File System
│       │   ├── block.h       # Block device layer
│       │   ├── ata.h         # ATA/IDE disk driver
│       │   ├── fat16.h       # FAT16 filesystem
│       │   └── syscall.h     # System calls
│       ├── kernel_entry.asm  # Kernel entry point
│       ├── kernel.c          # Kernel main function
│       ├── utils.asm         # Assembly utilities
│       ├── string.c          # String functions
│       ├── vga.c             # VGA display driver
│       ├── idt.c             # IDT implementation
│       ├── isr.asm           # ISR assembly part
│       ├── isr.c             # ISR C part
│       ├── pic.c             # PIC implementation
│       ├── pit.c             # PIT implementation
│       ├── keyboard.c        # Keyboard driver
│       ├── memory.c          # Physical memory manager
│       ├── paging.c          # Paging mechanism
│       ├── heap.c            # Heap allocator
│       ├── task.c            # Task scheduler
│       ├── shell.c           # Shell command line
│       ├── klog.c            # Kernel logging
│       ├── system.c          # System functions
│       ├── user.c            # User permissions
│       ├── vfs.c             # Virtual File System
│       ├── ramfs.c           # RAM File System
│       ├── block.c           # Block device layer
│       ├── ata.c             # ATA/IDE disk driver
│       ├── fat16.c           # FAT16 filesystem
│       ├── syscall.c         # System calls
│       └── legacy/           # Legacy code (backup)
│           ├── fs.c
│           ├── fs.h
│           ├── process.c
│           ├── process.h
│           └── process.asm
├── linker.ld                  # Kernel linker script
├── Makefile                   # Build script
├── README.md                  # Documentation
├── CHANGELOG.md               # This file
└── LICENSE                    # License
```

### 🔧 Technical Details
- **Compiler**: GCC (32-bit, freestanding)
- **Assembler**: NASM
- **Linker**: LD (ELF format)
- **Emulator**: QEMU (x86_64)
- **Target**: 32-bit x86 Protected Mode
- **Physical memory**: 128MB
- **Virtual address space**: 4GB
- **Page sizes**: 4KB (small), 4MB (large)

### 🚀 How to Build & Run
```bash
# Build
make build

# Run in QEMU
make run

# Debug with GDB
make debug

# Clean build artifacts
make clean
```

### 📈 Development Progress
- ✅ v0.1.0 - v0.2.0: Boot sector, real mode basics
- ✅ v0.3.0: 32-bit protected mode, C kernel, basic drivers
- ✅ v0.4.0: Memory isolation, large pages, page faults
- ✅ v0.5.0: User permissions, VFS, ramfs, disk driver, FAT16
- ⏳ v0.6.0: IPC, more drivers, FAT32
- ⏳ v0.7.0: Network support
- ⏳ v0.8.0: GUI
- ⏳ v1.0.0: Stable release

---

## [v0.4.0] - Previous Release
- Memory expansion (32MB → 128MB)
- Large page support (4MB)
- Kernel/user space isolation
- Page fault handling
- System reboot
- Kernel boot log

## [v0.3.0] - Previous Release
- 32-bit protected mode
- C language kernel
- VGA text output
- Interrupt system (IDT + ISR + PIC)
- PIT timer interrupt
- PS/2 keyboard driver
- Physical memory management (bitmap)
- Basic paging
- Heap allocator (kmalloc/kfree)
- Process scheduling (round robin)
- Shell command line
