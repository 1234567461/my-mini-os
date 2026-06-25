#!/bin/bash
# 创建MPK软件包的脚本

PKG_SOURCE_DIR="../packages/source"
PKG_REPO_DIR="../packages/repo"
MPK_TOOL="./mpk-tool"

mkdir -p "$PKG_SOURCE_DIR"
mkdir -p "$PKG_REPO_DIR"

# 工具列表：名称 描述
declare -a TOOLS=(
    "mpk:MPK package manager tool"
    "echo:Echo text to stdout"
    "cat:Concatenate and print files"
    "ls:List directory contents"
    "mkdir:Create directories"
    "rmdir:Remove empty directories"
    "touch:Change file timestamps"
    "cp:Copy files"
    "mv:Move or rename files"
    "rm:Remove files"
    "pwd:Print working directory"
    "cd:Change current directory"
    "head:Output first part of files"
    "tail:Output last part of files"
    "wc:Print newline, word, and byte counts"
    "grep:Print lines matching a pattern"
    "sort:Sort lines of text files"
    "cut:Remove sections from each line"
    "tr:Translate or delete characters"
    "rev:Reverse lines characterwise"
    "date:Print or set system date"
    "uname:Print system information"
    "whoami:Print current user name"
    "id:Print user and group information"
    "uptime:Tell how long system has been up"
    "hostname:Print system hostname"
    "true:Exit with success status"
    "false:Exit with failure status"
    "yes:Output a string repeatedly"
    "basename:Strip directory path"
    "dirname:Strip last component from path"
    "basename:Strip directory suffix"
)

echo "Creating packages..."

# 为每个工具创建源代码和目录
for tool_info in "${TOOLS[@]}"; do
    name="${tool_info%%:*}"
    desc="${tool_info##*:}"
    
    tool_dir="$PKG_SOURCE_DIR/$name"
    mkdir -p "$tool_dir/bin"
    mkdir -p "$tool_dir/share/man"
    
    echo "Creating $name - $desc"
    
    # 创建README
    cat > "$tool_dir/README" <<EOF
$name v1.0.0
$desc

Part of My Mini OS MPK package repository.
License: MIT
EOF

done

echo "Package source directories created."
