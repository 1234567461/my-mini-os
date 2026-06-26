/* ==========================================
 * version.h - 版本管理头文件
 * 功能：
 *   1. 版本信息存储和读取
 *   2. 版本比较
 *   3. 更新检查
 *   4. 过期提醒（3个月）
 * ========================================== */

#ifndef VERSION_H
#define VERSION_H

#include <stdint.h>
#include "types.h"

#define VERSION_MAX_LEN 32
#define VERSION_DATE_LEN 16

/* 版本信息结构 */
typedef struct {
    char version[VERSION_MAX_LEN];     /* 版本号：v1.1.0 */
    char build_date[VERSION_DATE_LEN]; /* 构建日期：2026-06-25 */
    char last_check[VERSION_DATE_LEN]; /* 最后检查日期 */
    uint32_t build_number;            /* 构建号 */
} version_info_t;

/* 版本状态 */
typedef enum {
    VERSION_OK = 0,           /* 已是最新 */
    VERSION_UPDATE_AVAILABLE,  /* 有可用更新 */
    VERSION_STALE,            /* 超过3个月未更新 */
    VERSION_ERROR             /* 检查失败 */
} version_status_t;

/* 初始化版本系统 */
void version_init(void);

/* 获取当前版本信息 */
version_info_t* version_get_current(void);

/* 获取最新版本信息（从仓库） */
version_status_t version_check_latest(char *latest_version, size_t buf_size);

/* 比较版本号：返回1表示v1>v2, 0表示相等, -1表示v1<v2 */
int version_compare(const char *v1, const char *v2);

/* 检查是否超过3个月未更新 */
int version_is_stale(void);

/* 获取距上次更新的天数 */
int version_days_since_update(void);

/* 更新最后检查时间 */
void version_update_check_time(void);

/* 显示版本信息 */
void version_print_info(void);

/* 显示更新状态 */
void version_print_update_status(void);

#endif /* VERSION_H */
