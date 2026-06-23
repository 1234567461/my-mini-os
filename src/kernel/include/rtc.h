/* ==========================================
 * 实时时钟（RTC）驱动头文件 - rtc.h
 * 功能：
 *   1. CMOS RTC 驱动
 *   2. 读取当前时间（年、月、日、时、分、秒）
 *   3. 读取 CMOS 信息
 * ========================================== */
#ifndef RTC_H
#define RTC_H

#include "types.h"

/* RTC 端口 */
#define RTC_PORT_INDEX  0x70
#define RTC_PORT_DATA   0x71

/* RTC 寄存器 */
#define RTC_REG_SECONDS     0x00
#define RTC_REG_MINUTES     0x02
#define RTC_REG_HOURS       0x04
#define RTC_REG_WEEKDAY     0x06
#define RTC_REG_DAY         0x07
#define RTC_REG_MONTH       0x08
#define RTC_REG_YEAR        0x09
#define RTC_REG_CENTURY     0x32  /* 世纪寄存器（ACPI） */
#define RTC_REG_STATUS_A    0x0A
#define RTC_REG_STATUS_B    0x0B
#define RTC_REG_STATUS_C    0x0C
#define RTC_REG_STATUS_D    0x0D

/* 状态寄存器A位 */
#define RTC_STATUS_A_UIP    0x80  /* 更新进行中 */

/* 状态寄存器B位 */
#define RTC_STATUS_B_24H    0x02  /* 24小时模式 */
#define RTC_STATUS_B_BINARY 0x04  /* 二进制模式（否则BCD） */
#define RTC_STATUS_B_SQWE   0x08  /* 方波使能 */
#define RTC_STATUS_B_UIE    0x10  /* 更新结束中断使能 */
#define RTC_STATUS_B_AIE    0x20  /* 闹钟中断使能 */
#define RTC_STATUS_B_PIE    0x40  /* 周期中断使能 */
#define RTC_STATUS_B_SET    0x80  /* 禁止更新 */

/* 时间结构体 */
typedef struct {
    uint8_t second;     /* 秒 (0-59) */
    uint8_t minute;     /* 分 (0-59) */
    uint8_t hour;       /* 时 (0-23) */
    uint8_t day;        /* 日 (1-31) */
    uint8_t month;      /* 月 (1-12) */
    uint16_t year;      /* 年 (如 2024) */
    uint8_t weekday;    /* 星期几 (0-6, 周日=0) */
} rtc_time_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化 RTC */
void rtc_init(void);

/* 读取 RTC 寄存器 */
uint8_t rtc_read_register(uint8_t reg);

/* 写入 RTC 寄存器 */
void rtc_write_register(uint8_t reg, uint8_t value);

/* 检查 RTC 是否正在更新 */
bool rtc_is_updating(void);

/* 读取当前时间 */
void rtc_get_time(rtc_time_t *time);

/* BCD 转二进制 */
uint8_t rtc_bcd_to_binary(uint8_t bcd);

/* 二进制转 BCD */
uint8_t rtc_binary_to_bcd(uint8_t binary);

/* 格式化时间字符串 */
void rtc_format_time(rtc_time_t *time, char *buf, int buf_size);

/* 格式化日期字符串 */
void rtc_format_date(rtc_time_t *time, char *buf, int buf_size);

#endif /* RTC_H */
