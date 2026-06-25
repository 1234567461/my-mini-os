# Changelog
All notable changes to this project will be documented in this file.

## [v1.1.0] - 2026-06-25
### Overview
v1.1.0 是 v1.0.0-a11y 之后的第一个功能更新版本，增加了包管理器、安装向导和SHA-512/256哈希验证功能。

> **重要更新**：
> - 包管理器：`pkg` 命令支持软件包搜索、安装、卸载
> - 安装向导：`setup-tui` / `setup-gui` 命令提供完整的系统安装流程
> - 哈希校验：内置SHA-512/256算法，用于验证下载文件的完整性
> - 所有镜像提供SHA-512/256哈希值，防止碰撞攻击

### 新增功能

#### 📦 包管理器（pkg）
- **搜索**：`pkg search <关键词>` - 搜索可用软件包
- **安装**：`pkg install <包名>` - 安装软件包
- **卸载**：`pkg remove <包名>` - 卸载软件包
- **列表**：`pkg list` - 列出已安装的包
- **信息**：`pkg info <包名>` - 显示包详情
- **更新**：`pkg update` - 更新所有软件包
- **刷新**：`pkg refresh` - 从仓库刷新包列表
- **清理**：`pkg clean` - 清理包缓存
- **依赖解析**：自动检查和安装依赖
- **仓库管理**：支持添加/移除自定义仓库

#### 🔧 安装向导（setup）
- **TUI版本**：`setup-tui` - 文本模式安装向导
- **GUI版本**：`setup-gui` - 图形模式安装向导
- **功能**：
  - 目标磁盘选择
  - 文件系统选择（FAT16/FAT32/RAM Disk）
  - 分区管理
  - 用户账户设置
  - 密码配置
  - 安装进度显示
  - 安装确认与回退

#### 🔒 SHA-512/256 哈希（防碰撞）
- **自研实现**：完全自主研发的哈希算法
- **防碰撞**：使用SHA-2家族的安全变体
- **内置命令**：`hash` / `verify` 命令
- **校验文件**：`img/SHA512SUMS.txt` 包含所有镜像的哈希值

#### 🔄 系统更新（update）
- **版本检测**：自动检测最新版本
- **更新检查**：`update check` - 检查更新
- **自动升级**：`update upgrade` - 升级到最新版本
- **过期提醒**：超过3个月未更新会强制提醒

### 内核更新
- 内核大小：173KB（相比v1.0.0增加约21KB）
- 新增模块：
  - hash.c/h - SHA-512/256哈希实现（~6KB）
  - pkg.c/h - 包管理器实现（~5KB）
  - setup.c/h - 安装向导实现（~10KB）
- 保持向后兼容

### 镜像文件
所有镜像的SHA-512/256哈希值保存在 `img/SHA512SUMS.txt`

| 版本 | 文件名 | SHA-512/256 哈希 |
|------|--------|------------------|
| v1.1.0-desktop | my-mini-os-v1.1.0-desktop.img | 2586071a6f5458... |
| v1.1.0-setup | my-mini-os-v1.1.0-setup.img | 6749f57469ef0b... |
| v1.1.0-terminal | my-mini-os-v1.1.0-terminal.img | 1f427e3a83ffcd... |
| v1.1.0 | my-mini-os-v1.1.0.img | 96a908877e7e32d7... |
| v1.0.0 | my-mini-os-v1.0.0.img | 1cf4626b3c91e68c... |
| v1.0.0-a11y | my-mini-os-v1.0.0-a11y.img | c668f6601a0e1e8e... |

#### 版本说明
- **v1.1.0-terminal**: 终端版本 - 启动后直接进入命令行Shell界面
- **v1.1.0-setup**: 安装向导版本 - 启动后直接进入Setup TUI安装向导
- **v1.1.0-desktop**: 桌面版本 - 启动后直接进入GUI图形桌面环境

### Development Progress
- [x] v0.1.0 - v0.2.0: Boot sector, real mode basics
- [x] v0.3.0: 32-bit protected mode, C kernel, basic drivers
- [x] v0.4.0: Memory isolation, large pages, page faults
- [x] v0.5.0: User permissions, VFS, ramfs, disk driver, FAT16
- [x] v0.6.0: IPC, device drivers (serial/RTC/mouse), FAT32, MBR, ELF loader, user lib
- [x] v0.7.0: Network support (TCP/IP, ARP, DHCP, UDP, ICMP, NE2000 driver)
- [x] v0.8.0: GUI (window manager, event system, graphics engine, taskbar, controls)
- [x] v0.9.0: Advanced GUI enhancements, network stack improvements
- [x] v1.0.0: Stable release with VBE/VESA graphics support 🎉
- [x] v1.0.0-a11y: Accessibility support (for visual/hearing/speech impairments) ♿
- [x] v1.1.0: Package manager, setup wizard, SHA-512/256 hash verification 📦🔧🔒
- [x] v1.1.0-mpk: 自研MPK包格式 + mpk-tool工具 + 示例软件包仓库 📦

---

## [v1.0.0-a11y] - 2026-06-25
### Overview
♿ 无障碍支持版本 - 为视障、听障、语障人士提供完善的辅助功能

> **特别说明**：这是一个具有社会意义的版本，致力于让每个人都能平等地使用计算技术。
> 内核仅增加约12KB（140KB → 152KB），极致压缩，不影响性能。

### 新增无障碍功能

#### 👁️ 视障辅助
- **高对比度模式** - 6种配色方案可选
  - 方案0：正常配色（黑底白字）
  - 方案1：高对比度（黑底黄字，最清晰）
  - 方案2：黑底白字（经典终端）
  - 方案3：白底黑字（纸质阅读感）
  - 方案4：黑底绿字（复古终端）
  - 方案5：低饱和度（护眼模式）
- **屏幕阅读器** - 通过COM1串口输出文字
  - 可外接专业屏幕阅读器设备
  - 115200波特率，标准串口协议
  - 所有文本内容自动同步输出
- **大字体模式** - 辅助低视力用户

#### 👂 听障辅助
- **视觉提示系统** - 用屏幕闪烁替代蜂鸣声
  - 可配置闪烁次数
  - 醒目颜色变化提示
- **视觉警报** - 重要事件的强烈视觉反馈
  - 红底黄字 + 闪烁边框
  - 确保不会错过任何提示

#### 🗣️ 语障辅助
- **预设短语系统** - 16个可自定义短语
  - 内置8个常用短语：你好、谢谢、是的、不是、请帮助我、我需要水、我需要上厕所、我饿了
  - 支持自定义短语内容
  - 一键输出到屏幕和串口
- **快速交流** - 数字键快速选择短语

#### 🛠️ 通用无障碍功能
- **`a11y` 命令** - 无障碍功能总控
  - `a11y help` - 帮助信息
  - `a11y status` - 当前状态查询
  - `a11y menu` - 交互式设置菜单
  - `a11y contrast` - 高对比度开关
  - `a11y largefont` - 大字体开关
  - `a11y screenread` - 屏幕阅读器开关
  - `a11y visualbeep` - 视觉提示开关
  - `a11y beep` - 测试视觉提示
  - `a11y scheme <n>` - 切换配色方案
- **`phrase` 命令** - 预设短语管理
  - `phrase list` - 列出所有短语
  - `phrase <n>` - 输出第n个短语
  - `phrase set <n> <text>` - 设置短语

### 性能与体积
- 内核体积增加：~12KB（140KB → 152KB）
- 镜像大小：1.44MB（不变）
- 启动时间：无影响
- 模块化设计：功能按需启用，不启用不消耗资源

### Development Progress
- [x] v0.1.0 - v0.2.0: Boot sector, real mode basics
- [x] v0.3.0: 32-bit protected mode, C kernel, basic drivers
- [x] v0.4.0: Memory isolation, large pages, page faults
- [x] v0.5.0: User permissions, VFS, ramfs, disk driver, FAT16
- [x] v0.6.0: IPC, device drivers (serial/RTC/mouse), FAT32, MBR, ELF loader, user lib
- [x] v0.7.0: Network support (TCP/IP, ARP, DHCP, UDP, ICMP, NE2000 driver)
- [x] v0.8.0: GUI (window manager, event system, graphics engine, taskbar, controls)
- [x] v0.9.0: Advanced GUI enhancements, network stack improvements
- [x] v1.0.0: Stable release with VBE/VESA graphics support 🎉
- [x] v1.0.0-a11y: Accessibility support (for visual/hearing/speech impairments) ♿

---

## [v1.0.0] - 2026-06-26
### Overview
v1.0.0 是 My Mini OS 的第一个正式稳定版本！经过多个阶段的开发，这个从零开始构建的操作系统已经具备了完整的内核、文件系统、网络协议栈和图形用户界面。

### 里程碑
- 🎉 **正式版发布**：从零开始的32位保护模式操作系统
- 🖼️ **VBE/VESA图形支持**：兼容标准VESA BIOS扩展的图形模式
- 📚 **完整功能集**：内核、文件系统、网络、GUI一应俱全

### v1.0.0 新增特性

#### VBE/VESA 图形模式支持
- **VBE检测与设置** (`loader.asm`):
  - 自动检测VBE/VESA BIOS扩展
  - 支持640x480x32 (模式0x118)
  - 降级支持：640x480x24 (0x117)、640x480x16 (0x114)
  - 线性帧缓冲（Linear Framebuffer）支持
- **VBE信息传递**：
  - Boot loader → 内核通过约定内存地址(0x7000)传递VBE参数
  - 动态分辨率、色深、帧缓冲地址传递
- **内核VBE驱动** (`vbe.c`):
  - VBE信息结构体定义
  - 帧缓冲地址可写性验证
  - 安全降级机制（VBE不可用时回退到VGA文本模式）
- **帧缓冲驱动增强** (`framebuffer.c`):
  - 动态分辨率支持（不再硬编码640x480）
  - 使用真实pitch（行间距）计算像素地址
  - 多色深支持（16/24/32 bpp）
- **Shell命令**：
  - 新增 `vbe` 命令：显示VBE图形模式信息

#### 硬件兼容性说明
> **重要提示**：本操作系统使用传统VBE/VESA (BIOS) 图形模式
> - 在现代UEFI系统上，需要在BIOS设置中开启 **CSM (Compatibility Support Module)**
> - 显卡需支持VBE/VESA DAC（模拟VGA输出）
> - 推荐使用VGA接口或支持Legacy模式的显示器
> - 纯UEFI GOP/HDMI/DP数字接口需要额外的驱动支持（未来版本）

### v1.0.0 完整功能清单
- ✅ **内核核心**
  - 32位保护模式（x86）
  - GDT/TSS 完整实现
  - 中断系统（IDT + ISR + PIC）
  - 物理内存管理（位图法）
  - 分页机制（4KB小页 + 4MB大页）
  - 堆分配器（kmalloc/kfree）
  - 进程调度（时间片轮转 + 优先级）
  - 系统调用（int 0x80）

- ✅ **文件系统**
  - 虚拟文件系统（VFS）抽象层
  - RAM文件系统（ramfs）
  - FAT16 文件系统
  - FAT32 文件系统
  - 块设备层 + 缓冲区缓存
  - ATA/IDE 磁盘驱动（PIO模式）
  - MBR分区表解析

- ✅ **网络协议栈**
  - NE2000 兼容网卡驱动
  - 以太网帧封装/解封装
  - ARP协议（地址解析）
  - IPv4协议（含校验和）
  - ICMP协议（ping）
  - UDP协议
  - TCP协议（简化版，含三次握手/四次挥手）
  - Socket API（BSD风格）
  - DHCP客户端（自动获取IP）

- ✅ **图形用户界面（GUI）**
  - 帧缓冲驱动（VBE/VESA模式）
  - 图形引擎（Bresenham算法）
  - 字体渲染（8x16位图字体）
  - 窗口管理器（拖动、焦点、z-order）
  - 事件系统（鼠标、键盘）
  - 任务栏 + 开始菜单
  - 控件库（按钮、文本框、进度条、复选框）
  - 高级图形（渐变、阴影、圆角矩形）

- ✅ **设备驱动**
  - VGA文本模式驱动
  - PS/2 键盘驱动
  - PS/2 鼠标驱动
  - PIT 可编程时钟
  - RTC 实时时钟
  - 串口驱动（COM1-COM4）

- ✅ **Shell命令行**
  - 50+ 内置命令
  - 文件操作：ls, cat, cp, mv, rm, mkdir, cd, pwd, touch, find, tree, wc, hexdump
  - 系统管理：ps, meminfo, dmesg, reboot, date, uptime, uname
  - 用户权限：su, whoami
  - 网络：ifconfig, ping, arp, dhcp, netstat
  - 图形：gui, vbe
  - 输入输出重定向（>、>>、<）
  - 脚本执行（run命令）

### Development Progress
- [x] v0.1.0 - v0.2.0: Boot sector, real mode basics
- [x] v0.3.0: 32-bit protected mode, C kernel, basic drivers
- [x] v0.4.0: Memory isolation, large pages, page faults
- [x] v0.5.0: User permissions, VFS, ramfs, disk driver, FAT16
- [x] v0.6.0: IPC, device drivers (serial/RTC/mouse), FAT32, MBR, ELF loader, user lib
- [x] v0.7.0: Network support (TCP/IP, ARP, DHCP, UDP, ICMP, NE2000 driver)
- [x] v0.8.0: GUI (window manager, event system, graphics engine, taskbar, controls)
- [x] v0.9.0: Advanced GUI enhancements, network stack improvements
- [x] v1.0.0: Stable release with VBE/VESA graphics support 🎉

---

## [v0.9.0] - 2026-06-25
### Overview
v0.9.0 是 v1.0.0 之前的最后一个预览版本，主要增强了GUI系统和网络栈的稳定性，并新增了高级shell功能。

### New Features

#### 高级 GUI 增强
- **高级图形效果**：
  - 渐变填充（垂直/水平）
  - 阴影效果
  - 圆角矩形
  - 抗锯齿像素
- **控件库扩展**：
  - 文本框控件（textbox_t）
  - 进度条控件（progressbar_t）
  - 复选框控件（checkbox_t）
  - 标签控件（label）
- **任务栏增强**：
  - 开始菜单
  - 系统托盘
  - 实时时钟显示
  - 窗口切换按钮

#### Shell 增强
- **高级文件命令**：
  - `cp` - 复制文件
  - `mv` - 移动/重命名文件
  - `ln` - 创建链接
  - `chmod` - 修改权限
  - `find` - 查找文件
  - `tree` - 目录树
  - `wc` - 统计行数/字符数
  - `hexdump` - 十六进制查看
- **输入输出重定向**：
  - `>` - 覆盖输出重定向
  - `>>` - 追加输出重定向
  - `<` - 输入重定向
- **脚本执行**：
  - `run` 命令执行脚本文件
  - 支持顺序执行多条命令

#### 网络栈完善
- TCP协议状态机优化
- Socket API 稳定性改进
- DHCP租约管理增强
- ARP缓存超时机制

### Bug Fixes
- 修复窗口拖动时的残影问题
- 修复TCP连接状态转换错误
- 修复shell命令解析的边界情况

---

## [v0.8.0] - 2026-06-24
### Overview
v0.8.0 is a major release that adds a complete Graphical User Interface (GUI) system with window management, event handling, and basic graphics rendering. Users can now launch a graphical desktop environment with draggable windows.

### New Features

#### GUI System (Graphical User Interface)
- **Framebuffer Driver** (`framebuffer.c`):
  - 640x480x32 graphics mode support
  - Pixel manipulation and area filling
  - Blit operations for bitmap transfer
  - Clip region support
- **Graphics Engine** (`graphics.c`):
  - Bresenham line drawing algorithm
  - Rectangle and filled rectangle drawing
  - Circle and filled circle drawing
  - Bitmap rendering for fonts and sprites
  - Clip region management
- **Font Renderer** (`font.c`):
  - 8x16 pixel bitmap font support
  - ASCII character rendering (0x20-0x7F)
  - Scalable font size (1x, 2x, etc.)
  - String drawing with newline support
- **Window Manager** (`window.c`):
  - Window creation with title bar and borders
  - Draggable windows (title bar)
  - Window focus management (click to focus)
  - Close button support
  - Window layering (z-order)
  - 3D-styled borders and title bars
- **Event System** (`event.c`):
  - Mouse move/down/up event handling
  - Window hit-testing (title bar, close button)
  - Event queue for asynchronous event processing
  - Drag and drop window support
- **GUI Main Loop** (`gui.c`):
  - Desktop window with gradient background
  - Demo windows with buttons and graphics
  - Event-driven window updates
  - Shell command `gui` to launch GUI

#### Shell Integration
- **New command**: `gui` - Launch graphical desktop environment
- **Command help**: Added GUI command to help text

#### Build System
- **New source files**: gui.c, font.c, framebuffer.c, graphics.c, window.c, event.c
- **Header file**: gui.h (comprehensive GUI definitions)

## [v0.7.0] - 2026-06-24
### Overview
v0.7.0 is a major release that adds proper user-mode execution support with GDT/TSS, improved ELF loading, and external program execution from shell. This version enables true user-space processes and lays the foundation for running user applications.

### New Features

#### GDT (Global Descriptor Table) and TSS (Task State Segment)
- **Complete GDT implementation**:
  - Kernel code/data segments (privilege level 0)
  - User code/data segments (privilege level 3)
  - TSS descriptor for task switching
  - gdt_flush() and tss_flush() assembly functions
- **TSS support**:
  - Kernel stack pointer tracking
  - Automatic kernel stack update on privilege level change
  - Hardware task switching support

#### User-Mode Execution
- **User-space address space**:
  - Separate user and kernel address spaces
  - User-space base: 0x40000000 (1GB)
  - User-space end: 0x80000000 (2GB)
  - Separate user stack and heap regions
- **User-mode context**:
  - Proper CS/DS segment selectors for user mode
  - iret-based return to user mode
  - User-mode stack management
- **System call gate**:
  - Ring 3 accessible interrupt gate (int 0x80)
  - Automatic kernel stack switching via TSS

#### External Program Execution
- **New shell command**: `exec <program> [args...]`
  - Execute programs from filesystem
  - Wait for program completion
  - Display exit status
- **sys_execve system call**:
  - ELF file loading and validation
  - User-space memory mapping
  - Process creation with user-mode context
- **sys_waitpid system call**:
  - Parent process waiting
  - Exit status reporting

#### Build System Improvements
- **Assembly/C object separation**:
  - Fixed Makefile to avoid name collisions
  - kernel_asm_*.o prefix for assembly objects
- **Compilation fixes**:
  - Proper function declarations and extern keywords
  - Fixed multiple definition errors

### Bug Fixes
- Fixed process_tick → task_tick rename issue
- Fixed pic_clear_mask → pic_irq_mask in mouse driver
- Fixed VGA_COLOR_YELLOW → VGA_COLOR_LIGHT_BROWN
- Fixed missing klog_logf function

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

#### File Descriptor Table
- **Per-process file descriptor table**: MAX_FDS (32) file descriptors per process
- **Standard file descriptors**: 0=stdin, 1=stdout, 2=stderr
- **File descriptor allocation**: Automatic allocation starting from fd=3
- **Integration with VFS**: File descriptors map to VFS file structures

#### System Call Enhancements (Part 2)
- **sys_open**: Complete implementation with VFS integration
  - Opens files using VFS interface
  - Allocates file descriptor from process table
  - Supports open flags (RDONLY, WRONLY, RDWR, CREAT, TRUNC)
- **sys_close**: Complete implementation
  - Closes file via VFS
  - Frees file descriptor slot
  - Validates file descriptor range
- **sys_read enhanced**: Supports regular file descriptors
  - fd=0 reads from keyboard (stdin)
  - fd>=3 reads from VFS files
- **sys_write enhanced**: Supports regular file descriptors
  - fd=1,2 writes to VGA (stdout/stderr)
  - fd>=3 writes to VFS files
- **sys_execve**: Simplified implementation
  - Loads ELF executable from file system
  - Replaces current process execution flow
  - Supports ELF32 format
- **sys_waitpid**: Simplified implementation
  - Waits for child process termination
  - Reaps zombie processes
  - Returns exit status

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
- [x] v0.7.0: Network support (TCP/IP, ARP, DHCP, UDP, ICMP, NE2000 driver)
- [x] v0.8.0: GUI (window manager, event system, graphics engine, taskbar, controls)
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
- [x] v0.6.0: IPC, more drivers, FAT32
- [x] v0.7.0: Network support
- [x] v0.8.0: GUI
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
