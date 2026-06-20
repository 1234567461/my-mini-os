# ==========================================
# My Mini OS - Makefile
# ==========================================

# 工具
NASM = nasm
QEMU = qemu-system-x86_64

# 目录
SRC_DIR = src
BOOT_DIR = $(SRC_DIR)/boot
BUILD_DIR = build

# 输出文件
BOOT_BIN = $(BUILD_DIR)/bootsect.bin
OS_IMG = $(BUILD_DIR)/myos.img

# 默认目标
all: build

# 创建build目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 编译引导扇区
$(BOOT_BIN): $(BOOT_DIR)/bootsect.asm | $(BUILD_DIR)
	$(NASM) -f bin $< -o $@
	@echo "✅ 引导扇区编译完成: $@"
	@echo "📦 文件大小: $$(wc -c < $@) 字节"

# 构建OS镜像
build: $(BOOT_BIN)
	@echo ""
	@echo "🎉 OS镜像构建完成!"
	@echo "💿 镜像文件: $(OS_IMG)"

# 在QEMU中运行
run: build
	@echo ""
	@echo "🚀 启动QEMU虚拟机..."
	$(QEMU) -drive format=raw,file=$(BOOT_BIN) -nographic

# 调试模式（带GDB）
debug: build
	@echo ""
	@echo "🔧 启动QEMU调试模式..."
	@echo "   在另一个终端运行: gdb -ex 'target remote :1234'"
	$(QEMU) -drive format=raw,file=$(BOOT_BIN) -s -S -nographic

# 查看二进制内容
hexdump: $(BOOT_BIN)
	@echo ""
	@echo "📋 引导扇区二进制内容:"
	xxd $(BOOT_BIN) | head -32
	@echo "..."
	@echo ""
	@echo "📍 最后两个字节（引导魔数）:"
	xxd $(BOOT_BIN) | tail -2

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
