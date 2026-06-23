# ==========================================
# My Mini OS - Makefile
# ==========================================

# 工具
NASM    = nasm
GCC     = gcc
LD      = ld
OBJCOPY = objcopy
QEMU    = qemu-system-x86_64
DD      = dd

# 编译选项
GCC_FLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -c -Isrc/kernel/include
LD_FLAGS  = -m elf_i386 -T linker.ld

# 目录
SRC_DIR    = src
BOOT_DIR   = $(SRC_DIR)/boot
KERNEL_DIR = $(SRC_DIR)/kernel
BUILD_DIR  = build

# 输出文件
BOOT_BIN   = $(BUILD_DIR)/bootsect.bin
LOADER_BIN = $(BUILD_DIR)/loader.bin
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
OS_IMG     = $(BUILD_DIR)/myos.img

# 内核源文件
KERNEL_ASM_SRCS = $(wildcard $(KERNEL_DIR)/*.asm)
KERNEL_C_SRCS   = $(wildcard $(KERNEL_DIR)/*.c)
KERNEL_OBJS     = $(patsubst $(KERNEL_DIR)/%.asm, $(BUILD_DIR)/kernel_%.o, $(KERNEL_ASM_SRCS)) \
                  $(patsubst $(KERNEL_DIR)/%.c, $(BUILD_DIR)/kernel_%.o, $(KERNEL_C_SRCS))

# 默认目标
all: build

# 创建build目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ====== 引导扇区（第一阶段） ======
$(BOOT_BIN): $(BOOT_DIR)/bootsect.asm | $(BUILD_DIR)
	$(NASM) -f bin $< -o $@
	@echo "✅ 引导扇区编译完成: $@"
	@echo "   文件大小: $$(wc -c < $@) 字节"

# ====== 加载器（第二阶段） ======
$(LOADER_BIN): $(BOOT_DIR)/loader.asm | $(BUILD_DIR)
	$(NASM) -f bin $< -o $@
	@echo "✅ 第二阶段加载器编译完成: $@"
	@echo "   文件大小: $$(wc -c < $@) 字节"

# ====== 内核：汇编文件编译 ======
$(BUILD_DIR)/kernel_%.o: $(KERNEL_DIR)/%.asm | $(BUILD_DIR)
	$(NASM) -f elf32 $< -o $@
	@echo "✅ 内核汇编编译: $<"

# ====== 内核：C文件编译 ======
$(BUILD_DIR)/kernel_%.o: $(KERNEL_DIR)/%.c | $(BUILD_DIR)
	$(GCC) $(GCC_FLAGS) $< -o $@
	@echo "✅ 内核C编译: $<"

# ====== 内核：链接 ======
$(KERNEL_ELF): $(KERNEL_OBJS) linker.ld
	$(LD) $(LD_FLAGS) $(KERNEL_OBJS) -o $@
	@echo "✅ 内核链接完成: $@"

# ====== 内核：转换为纯二进制 ======
$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@
	@echo "✅ 内核二进制生成: $@"
	@echo "   文件大小: $$(wc -c < $@) 字节"

# ====== 构建完整OS镜像 ======
$(OS_IMG): $(BOOT_BIN) $(LOADER_BIN) $(KERNEL_BIN)
	@echo ""
	@echo "🔨 构建OS镜像..."

	# 创建一个空的1.44MB软盘镜像
	$(DD) if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null

	# 写入引导扇区（第1个扇区）
	$(DD) if=$(BOOT_BIN) of=$@ bs=512 count=1 conv=notrunc 2>/dev/null

	# 写入第二阶段加载器（第2-5个扇区，共4个扇区）
	$(DD) if=$(LOADER_BIN) of=$@ bs=512 seek=1 count=4 conv=notrunc 2>/dev/null

	# 写入内核（从第6个扇区开始）
	$(DD) if=$(KERNEL_BIN) of=$@ bs=512 seek=5 conv=notrunc 2>/dev/null

	@echo ""
	@echo "🎉 OS镜像构建完成!"
	@echo "💿 镜像文件: $@"
	@echo "📦 镜像大小: $$(wc -c < $@) 字节"
	@echo ""
	@echo "📊 各部分大小:"
	@echo "   Bootloader: $$(wc -c < $(BOOT_BIN)) 字节 (1 扇区)"
	@echo "   Loader:     $$(wc -c < $(LOADER_BIN)) 字节 (最多 4 扇区)"
	@echo "   Kernel:     $$(wc -c < $(KERNEL_BIN)) 字节 (从第 6 扇区开始)"

# 构建
build: $(OS_IMG)

# 在QEMU中运行
run: build
	@echo ""
	@echo "🚀 启动QEMU虚拟机..."
	$(QEMU) -drive format=raw,file=$(OS_IMG) -nographic

# 调试模式（带GDB）
debug: build
	@echo ""
	@echo "🔧 启动QEMU调试模式..."
	@echo "   在另一个终端运行: gdb -ex 'target remote :1234'"
	$(QEMU) -drive format=raw,file=$(OS_IMG) -s -S -nographic

# 查看二进制内容
hexdump: $(OS_IMG)
	@echo ""
	@echo "📋 引导扇区（前512字节）:"
	xxd $(BOOT_BIN) | head -20
	@echo "..."
	@echo ""
	@echo "📍 引导扇区最后两个字节（魔数）:"
	xxd $(BOOT_BIN) | tail -2
	@echo ""
	@echo "📋 内核开头:"
	xxd $(KERNEL_BIN) | head -10

# 清理
clean:
	rm -rf $(BUILD_DIR)
	@echo "🧹 清理完成"

# 帮助
help:
	@echo "My Mini OS - 构建系统"
	@echo ""
	@echo "用法: make [目标]"
	@echo ""
	@echo "目标:"
	@echo "  all       - 构建所有（默认）"
	@echo "  build     - 构建OS镜像"
	@echo "  run       - 在QEMU中运行"
	@echo "  debug     - 调试模式（GDB远程调试）"
	@echo "  hexdump   - 查看二进制内容"
	@echo "  clean     - 清理构建文件"
	@echo "  help      - 显示帮助信息"

.PHONY: all build run debug hexdump clean help
