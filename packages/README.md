# MPK - My Mini OS Package Format

## 概述
MPK 是 My Mini OS 自研的包文件格式，用于软件分发和安装。

## 文件格式

### 文件结构
```
+-----------------------+  <-- 0
|   MPK File Header     |   64 bytes
+-----------------------+  <-- 64
|   Metadata Section    |   可变长度
|   (键值对格式)        |
+-----------------------+  <-- header.meta_offset + header.meta_size
|   File Table          |   文件条目表
+-----------------------+  <-- header.file_table_offset
|   File Data           |   文件内容
+-----------------------+  <-- 文件末尾
```

### 头部结构 (64 bytes)
| 偏移 | 大小 | 字段 | 说明 |
|------|------|------|------|
| 0 | 4 | magic | 魔数 0x4B504D4B ("KPMK") |
| 4 | 2 | version | 格式版本 (当前: 1) |
| 6 | 2 | flags | 标志位 |
| 8 | 4 | header_size | 头部大小 (64) |
| 12 | 4 | meta_offset | 元数据偏移 |
| 16 | 4 | meta_size | 元数据大小 |
| 20 | 4 | file_table_offset | 文件表偏移 |
| 24 | 4 | file_count | 文件数量 |
| 28 | 4 | data_offset | 数据区偏移 |
| 32 | 4 | data_size | 数据区大小（压缩后） |
| 36 | 4 | data_size_orig | 数据区原始大小 |
| 40 | 1 | compress_algo | 压缩算法 |
| 41 | 19 | reserved | 保留 |
| 60 | 4 | checksum | 文件CRC32校验和 |

### 元数据格式
使用简单的键值对格式，每行一个：
```
name=hello
version=1.0.0
description=Hello World demo program
author=mpk-tool
dep0=libc
dep1=libm
```

### 文件条目结构 (88 bytes)
| 偏移 | 大小 | 字段 | 说明 |
|------|------|------|------|
| 0 | 64 | name | 文件名（含路径） |
| 64 | 4 | offset | 在数据区的偏移 |
| 68 | 4 | size | 文件大小（压缩后） |
| 72 | 4 | size_orig | 文件原始大小 |
| 76 | 4 | mode | 文件权限 |
| 80 | 4 | flags | 文件标志 |
| 84 | 4 | checksum | 文件内容CRC32校验和 |

## 压缩算法
- 0: 不压缩
- 1: RLE (游程编码)
- 2: LZSS (预留)

## 工具
使用 `tools/mpk-tool` 创建、查看、提取MPK包：

```bash
# 创建包
mpk-tool create output.mpk file1 file2 ...

# 查看包信息
mpk-tool info package.mpk

# 列出包内容
mpk-tool list package.mpk

# 提取包
mpk-tool extract package.mpk [output_dir]

# 转换目录为包
mpk-tool convert input_dir output.mpk --name hello --version 1.0.0
```

## 内核命令
在 My Mini OS 中使用 `mpk` 命令：

```
mpk info <file.mpk>    - 显示包信息
mpk list <file.mpk>    - 列出包内容
mpk verify <file.mpk>  - 验证包校验和
mpk extract <file.mpk> - 提取包内容
```

## 校验和
使用CRC32算法验证文件完整性。校验和存储在头部最后4字节，计算时该字段设为0。
