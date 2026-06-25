/* ==========================================
 * 安装向导 - Setup Wizard
 * 提供命令行和GUI两种安装模式
 * ========================================== */

#ifndef SETUP_H
#define SETUP_H

#include "types.h"

/* 安装向导状态 */
typedef enum {
    SETUP_WELCOME = 0,
    SETUP_LANGUAGE,
    SETUP_TARGET,
    SETUP_PARTITION,
    SETUP_FILESYSTEM,
    SETUP_CONFIRM,
    SETUP_INSTALLING,
    SETUP_FINISH
} setup_state_t;

/* 安装选项 */
typedef struct {
    char language[16];          /* 语言选择 */
    int target_disk;            /* 目标磁盘编号 */
    int partition_number;       /* 分区编号 */
    char filesystem[16];        /* 文件系统类型 */
    int install_mbr;            /* 是否安装MBR */
    int quick_mode;             /* 快速模式（跳过确认） */
} setup_options_t;

/* ==========================================
 * 安装向导入口
 * ========================================== */

/* 启动安装向导（TUI版本） */
void setup_wizard_tui(void);

/* 启动安装向导（GUI版本） */
void setup_wizard_gui(void);

/* 安装进度回调 */
typedef void (*setup_progress_callback_t)(int percentage, const char *message);
void setup_set_progress_callback(setup_progress_callback_t callback);

/* 执行实际安装 */
int setup_execute_install(setup_options_t *options);

/* 获取安装向导版本 */
const char *setup_get_version(void);

#endif /* SETUP_H */
