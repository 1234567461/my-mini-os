# Changelog
All notable changes to this project will be documented in this file.

## [v0.6.0] - 2026-06-23
### Overview
v0.6.0 is a major release that adds comprehensive device driver support, FAT32 file system, MBR partition table parsing, and inter-process communication (IPC). This version significantly expands the hardware support and process management capabilities of the operating system.

### New Features

#### Disk Partition Support (MBR)
- **MBR partition table parsing**: Complete Master Boot Record parsing
  - Read and validate MBR signature (0xAA55)
  - Parse 4 primary partitions
  - Support for CHS and LBA addressing
  - Active partition detection
- **Partition type identification**: Recognize common partition types
  - FAT12/FAT16/FAT32
  - NTFS, Linux swap, Linux ext
  - Extended partitions
- **New shell command**: `partitions` - Display disk partition table

#### FAT32 File System
- **Complete FAT32 implementation**:
  - Boot sector/BPB parsing
  - FSInfo sector support
  - FAT table read/write operations
  - Cluster allocation and deallocation
  - Cluster chain management
- **VFS compatible**: Implements all VFS operations
- **Root directory support**: Full file operations in root directory
- **Large file support**: Supports files larger than FAT16 limits

#### Inter-Process Communication (IPC)
- **Pipe communication**:
  - Ring buffer implementation (4KB buffer)
  - Read/write file descriptor interface
  - Non-blocking mode support
  - Pipe creation, read, write, close operations
- **Signal mechanism**:
  - 32 signal types (SIGHUP, SIGINT, SIGKILL, etc.)
  - Signal handler registration
  - Signal sending (kill, raise)
  - Signal blocking (sigprocmask)
  - Pending signal checking and processing
  - SIGKILL and SIGSTOP cannot be caught or blocked

#### Process Priority Scheduling
- **Priority-based time slices**: Higher priority processes get more CPU time
- **Time slice calculation**: `time_slice = DEFAULT_TIME_SLICE + priority * 5`
- **Priority levels**: 0-31 priority levels
- **Fair scheduling**: Still round-robin based, but with variable time slices

#### Serial Port Driver (COM1-COM4)
- **Full serial port support**: COM1, COM2, COM3, COM4
- **Baud rate configuration**: Support for common baud rates
  - 115200, 57600, 38400, 19200, 9600, etc.
- **Serial parameters**: 8 data bits, 1 stop bit, no parity (8N1)
- **FIFO buffer support**: Hardware FIFO for better performance
- **I/O operations**:
  - Blocking and non-blocking read/write
  - Character and string output
  - Formatted output (serial_printf)
- **Port addresses**:
  - COM1: 0x3F8
  - COM2: 0x2F8
  - COM3: 0x3E8
  - COM4: 0x2E8
- **New shell command**: `serial` - Serial port test and text output

#### Real-Time Clock (RTC) Driver
- **CMOS RTC reading**: Read time from hardware RTC
- **Time/date information**:
  - Year, month, day
  - Hour, minute, second
  - Day of week
- **BCD conversion**: BCD to binary and binary to BCD conversion
- **Time formatting**:
  - Date format: YYYY-MM-DD
  - Time format: HH:MM:SS
- **New shell command**: `date` - Display current date and time

#### PS/2 Mouse Driver
- **PS/2 mouse support**: IRQ12 interrupt driven
- **3-byte packet parsing**: Standard PS/2 mouse protocol
- **Movement detection**: X and Y axis delta tracking
- **Button detection**: Left, right, and middle button states
- **Callback mechanism**: Register callback functions for mouse events
- **Mouse state structure**: Complete state information
  - Position coordinates
  - Movement delta
  - Button states (pressed/released)
- **New shell command**: `mouse` - Display mouse status

### Shell Commands
**New commands added in v0.6.0:**
- `date` - Show current date and time (RTC)
- `partitions` - Show disk partition table (MBR)
- `mouse` - Show mouse status
- `serial` - Serial port test, send text to COM1

### New Files Added
```
src/kernel/include/
├── serial.h      # Serial port driver header
├── rtc.h         # RTC driver header
├── mouse.h       # PS/2 mouse driver header
├── mbr.h         # MBR partition table header
├── fat32.h       # FAT32 filesystem header
└── ipc.h         # IPC (pipes & signals) header

src/kernel/
├── serial.c      # Serial port driver implementation
├── rtc.c         # RTC driver implementation
├── mouse.c       # PS/2 mouse driver implementation
├── mbr.c         # MBR partition table implementation
├── fat32.c       # FAT32 filesystem implementation
└── ipc.c         # IPC (pipes & signals) implementation
```

### Technical Details
- **Compiler**: GCC (32-bit, freestanding)
- **Assembler**: NASM
- **Linker**: LD (ELF format)
- **Emulator**: QEMU (x86_64)
- **Target**: 32-bit x86 Protected Mode
- **Physical memory**: 128MB
- **Virtual address space**: 4GB
- **Page sizes**: 4KB (small), 4MB (large)
- **Interrupts**: IRQ12 (mouse), IRQ8 (RTC)

### Enhanced Features (v0.6.0 Update)
#### FAT32 File System Enhancements
- **File write support**: Complete write implementation with cluster chain extension
- **File creation**: Create new files in root directory
- **File deletion**: Delete files and free data clusters
- **Directory creation**: Create subdirectories in root directory
- **Directory deletion**: Remove empty directories
- **Helper functions**:
  - `fat32_extend_chain()` - Extend file cluster chain
  - `fat32_update_dir_entry()` - Update directory entry metadata

#### System Call Improvements
- **sys_read**: Implemented standard input reading (fd=0) from keyboard
- **sys_brk**: Implemented user heap adjustment
  - Returns current brk when addr=0
  - Validates address range
  - Updates heap boundary pointer
- **sys_fork**: Simplified fork implementation
  - Creates new kernel process
  - Shares address space (simplified)
  - Returns child PID to parent

#### Signal Processing Integration
- **Scheduler integration**: Signal checking in `task_tick()`
- **Pending signal processing**: Signals are checked every clock tick
- **Default signal handlers**: Termination for fatal signals, ignore for others

#### User Space Library
- **User-mode headers**:
  - `syscall.h` - System call numbers and macros (syscall0-syscall3)
  - `unistd.h` - Unix-style function wrappers (exit, fork, read, write, open, close, getpid, sbrk)
  - `string.h` - String and memory operations (strlen, strcpy, strcmp, memcpy, memset, etc.)
- **Inline implementations**: All functions as static inline for easy use

#### ELF Loader
- **ELF32 support**: 32-bit ELF executable format
- **File validation**: Magic number, class, data encoding, version checks
- **Program segment loading**: Load PT_LOAD segments into memory
- **BSS support**: Zero-initialize uninitialized data
- **Entry point extraction**: Get program entry point address

### New Files Added (Update)
```
src/user/include/
├── syscall.h     # User-mode syscall definitions & macros
├── unistd.h      # Unix-style function wrappers
└── string.h      # String & memory operations

src/kernel/include/
└── elf.h         # ELF file format header

src/kernel/
└── elf.c         # ELF loader implementation
```

### Development Progress
- [x] v0.1.0 - v0.2.0: Boot sector, real mode basics
- [x] v0.3.0: 32-bit protected mode, C kernel, basic drivers
- [x] v0.4.0: Memory isolation, large pages, page faults
- [x] v0.5.0: User permissions, VFS, ramfs, disk driver, FAT16
- [x] v0.6.0: IPC, device drivers (serial/RTC/mouse), FAT32, MBR, ELF loader, user lib
- [ ] v0.7.0: Network support
- [ ] v0.8.0: GUI
- [ ] v1.0.0: Stable release

---

## [v0.5.0] - 2026-06-23
### Overview
v0.5.0 is a major release that introduces memory isolation, user permissions, virtual file system, and disk support. This version lays the foundation for a more secure and capable operating system.

### New Features

#### Memory Management & Protection
- **Extended physical memory**: 32MB → 128MB (32768 pages)
- **Large page support**: 4MB large pages (PSE) for kernel space, reducing TLB misses
- **Full virtual memory**: Complete two-level page table implementation
- **Kernel/User space isolation**: 
  - Kernel space: 0x00000000 - 0x003FFFFF (4MB, Ring 0 exclusive)
  - User space: 0x00400000 - 0x07FFFFFF (124MB, Ring 3 accessible)
- **Independent address spaces**: Each process has its own page directory (CR3 switching)
- **Page fault handling**: Captures illegal memory access with detailed error info
- **Independent stacks**: Separate kernel stack and user stack for each process

#### User Permission System
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

#### Virtual File System (VFS)
- **VFS abstraction layer**: Unified interface for multiple file systems
- **File system operations**: open, close, read, write, create, unlink, mkdir, rmdir, readdir, stat
- **File types**: Regular files and directories
- **Path resolution**: Relative and absolute path support

#### RAM File System (ramfs)
- **In-memory file system**: Tree-structured directory hierarchy
- **Full file operations**: create, read, write, delete
- **Directory support**: mkdir, rmdir, cd, pwd
- **Shell commands**: ls, cat, mkdir, rm, cd, pwd, touch

#### Disk & Block Device Layer
- **Block device abstraction**: Unified block device interface
- **Buffer cache**: 64-block LRU cache for improved disk I/O performance
- **Dirty buffer tracking**: Write-back caching with block_sync()
- **ATA/IDE disk driver**: PIO mode support
  - Read/write sectors
  - Drive identification
  - Primary channel support

#### FAT16 File System
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

#### System Enhancements
- **System reboot**: `reboot` command via 8042 keyboard controller
- **Kernel log**: `dmesg` command with ring buffer
  - Boot logging for all subsystems
  - Multiple log levels
- **System call framework**: int 0x80 soft interrupt
  - Basic syscalls: exit, write, read, open, close, getpid, brk
  - Parameter passing via registers

### Architecture Improvements
- 32-bit x86 Protected Mode
- Separate kernel and user address spaces
- Memory protection via page permissions
- Modular driver architecture
- Layered file system design (VFS → FS → Block Device)

### Shell Commands
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

### Project Structure
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

### Technical Details
- **Compiler**: GCC (32-bit, freestanding)
- **Assembler**: NASM
- **Linker**: LD (ELF format)
- **Emulator**: QEMU (x86_64)
- **Target**: 32-bit x86 Protected Mode
- **Physical memory**: 128MB
- **Virtual address space**: 4GB
- **Page sizes**: 4KB (small), 4MB (large)

### How to Build & Run
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

### Development Progress
- [x] v0.1.0 - v0.2.0: Boot sector, real mode basics
- [x] v0.3.0: 32-bit protected mode, C kernel, basic drivers
- [x] v0.4.0: Memory isolation, large pages, page faults
- [x] v0.5.0: User permissions, VFS, ramfs, disk driver, FAT16
- [ ] v0.6.0: IPC, more drivers, FAT32
- [ ] v0.7.0: Network support
- [ ] v0.8.0: GUI
- [ ] v1.0.0: Stable release

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
