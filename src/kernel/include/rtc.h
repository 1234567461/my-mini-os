/**
 * @file rtc.h
 * @brief 实时时钟（RTC）驱动
 * @version v0.6.0
 *
 * 通过 CMOS RTC 芯片读取系统时间
 */

#ifndef RTC_H
#define RTC_H

#include <types.h>

// RTC CMOS 端口
#define RTC_ADDRESS_PORT    0x70
#define RTC_DATA_PORT       0x71

// RTC 寄存器
#define RTC_REG_SECONDS     0x00
#define RTC_REG_MINUTES     0x02
#define RTC_REG_HOURS       0x04
#define RTC_REG_WEEKDAY     0x06
#define RTC_REG_DAY         0x07
#define RTC_REG_MONTH       0x08
#define RTC_REG_YEAR        0x09
#define RTC_REG_CENTURY     0x32    // 世纪寄存器（可选）
#define RTC_REG_STATUS_A    0x0A
#define RTC_REG_STATUS_B    0x0B
#define RTC_REG_STATUS_C    0x0C
#define RTC_REG_STATUS_D    0x0D

// 状态寄存器 A 位
#define RTC_STATUS_A_UIP    0x80    // 更新进行中
#define RTC_STATUS_A_DV2    0x40    // 分频器位 2
#define RTC_STATUS_A_DV1    0x20    // 分频器位 1
#define RTC_STATUS_A_DV0    0x10    // 分频器位 0
#define RTC_STATUS_A_RS3    0x08    // 速率选择位 3
#define RTC_STATUS_A_RS2    0x04    // 速率选择位 2
#define RTC_STATUS_A_RS1    0x02    // 速率选择位 1
#define RTC_STATUS_A_RS0    0x01    // 速率选择位 0

// 状态寄存器 B 位
#define RTC_STATUS_B_SET    0x80    // 停止更新
#define RTC_STATUS_B_PIE    0x40    // 周期中断使能
#define RTC_STATUS_B_AIE    0x20    // 闹钟中断使能
#define RTC_STATUS_B_UIE    0x10    // 更新结束中断使能
#define RTC_STATUS_B_SQWE   0x08    // 方波使能
#define RTC_STATUS_B_DM     0x04    // 数据模式（1=二进制，0=BCD）
#define RTC_STATUS_B_24_12  0x02    // 24/12 小时模式（1=24小时）
#define RTC_STATUS_B_DSE    0x01    // 夏令时使能

// 状态寄存器 C 位
#define RTC_STATUS_C_IRQF   0x80    // 中断请求标志
#define RTC_STATUS_C_PF     0x40    // 周期中断标志
#define RTC_STATUS_C_AF     0x20    // 闹钟中断标志
#define RTC_STATUS_C_UF     0x10    // 更新结束中断标志

// 状态寄存器 D 位
#define RTC_STATUS_D_VRT    0x80    // 有效 RAM/时间

/**
 * @brief 时间结构体
 */
typedef struct {
    uint8_t second;     // 秒 (0-59)
    uint8_t minute;     // 分 (0-59)
    uint8_t hour;       // 时 (0-23)
    uint8_t weekday;    // 星期 (1-7, 1=周日)
    uint8_t day;        // 日 (1-31)
    uint8_t month;      // 月 (1-12)
    uint16_t year;      // 年 (如 2024)
} rtc_time_t;

/**
 * @brief 初始化 RTC
 * @return 成功返回 0
 */
int rtc_init(void);

/**
 * @brief 读取 RTC 寄存器
 * @param reg 寄存器号
 * @return 寄存器值
 */
uint8_t rtc_read_register(uint8_t reg);

/**
 * @brief 写入 RTC 寄存器
 * @param reg 寄存器号
 * @param value 要写入的值
 */
void rtc_write_register(uint8_t reg, uint8_t value);

/**
 * @brief 检查 RTC 是否正在更新
 * @return 正在更新返回 true
 */
bool rtc_is_updating(void);

/**
 * @brief 获取当前时间
 * @param time 输出时间结构体
 * @return 成功返回 0
 */
int rtc_get_time(rtc_time_t *time);

/**
 * @brief 格式化时间字符串
 * @param time 时间结构体
 * @param buf 输出缓冲区
 * @param max_len 缓冲区大小
 * @return 字符串长度
 */
int rtc_format_time(const rtc_time_t *time, char *buf, int max_len);

/**
 * @brief 格式化日期字符串
 * @param time 时间结构体
 * @param buf 输出缓冲区
 * @param max_len 缓冲区大小
 * @return 字符串长度
 */
int rtc_format_date(const rtc_time_t *time, char *buf, int max_len);

/**
 * @brief 获取星期名称
 * @param weekday 星期数 (1-7)
 * @return 星期名称字符串
 */
const char *rtc_get_weekday_name(uint8_t weekday);

/**
 * @brief 获取月份名称
 * @param month 月份 (1-12)
 * @return 月份名称字符串
 */
const char *rtc_get_month_name(uint8_t month);

#endif // RTC_H
