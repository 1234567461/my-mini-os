/**
 * @file rtc.c
 * @brief 实时时钟（RTC）驱动实现
 * @version v0.6.0
 */

#include <rtc.h>
#include <types.h>
#include <vga.h>
#include <string.h>

// 星期名称
static const char *weekday_names[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", 
    "Thursday", "Friday", "Saturday"
};

// 月份名称
static const char *month_names[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

/**
 * @brief 读取 RTC 寄存器
 */
uint8_t rtc_read_register(uint8_t reg)
{
    outb(RTC_ADDRESS_PORT, reg);
    return inb(RTC_DATA_PORT);
}

/**
 * @brief 写入 RTC 寄存器
 */
void rtc_write_register(uint8_t reg, uint8_t value)
{
    outb(RTC_ADDRESS_PORT, reg);
    outb(RTC_DATA_PORT, value);
}

/**
 * @brief 检查 RTC 是否正在更新
 */
bool rtc_is_updating(void)
{
    return (rtc_read_register(RTC_REG_STATUS_A) & RTC_STATUS_A_UIP) != 0;
}

/**
 * @brief BCD 转二进制
 */
static inline uint8_t bcd_to_binary(uint8_t bcd)
{
    return (bcd & 0x0F) + ((bcd >> 4) * 10);
}

/**
 * @brief 初始化 RTC
 */
int rtc_init(void)
{
    // 读取状态寄存器 B
    uint8_t status_b = rtc_read_register(RTC_REG_STATUS_B);
    
    // 检查是否是 BCD 模式
    bool is_bcd = !(status_b & RTC_STATUS_B_DM);
    
    // 检查是否是 24 小时模式
    bool is_24_hour = (status_b & RTC_STATUS_B_24_12) != 0;
    
    vga_printf("[RTC] RTC initialized (%s mode, %s-hour)\n",
              is_bcd ? "BCD" : "Binary",
              is_24_hour ? "24" : "12");
    
    // 测试读取时间
    rtc_time_t time;
    if (rtc_get_time(&time) == 0) {
        char time_str[32];
        char date_str[32];
        rtc_format_time(&time, time_str, sizeof(time_str));
        rtc_format_date(&time, date_str, sizeof(date_str));
        vga_printf("[RTC] Current time: %s %s\n", date_str, time_str);
    }
    
    return 0;
}

/**
 * @brief 获取当前时间
 */
int rtc_get_time(rtc_time_t *time)
{
    if (!time) {
        return -1;
    }
    
    // 等待更新完成
    while (rtc_is_updating()) {
        // 忙等待
    }
    
    // 读取时间（BCD 格式）
    uint8_t second = rtc_read_register(RTC_REG_SECONDS);
    uint8_t minute = rtc_read_register(RTC_REG_MINUTES);
    uint8_t hour = rtc_read_register(RTC_REG_HOURS);
    uint8_t weekday = rtc_read_register(RTC_REG_WEEKDAY);
    uint8_t day = rtc_read_register(RTC_REG_DAY);
    uint8_t month = rtc_read_register(RTC_REG_MONTH);
    uint8_t year = rtc_read_register(RTC_REG_YEAR);
    
    // 读取状态寄存器 B，判断数据格式
    uint8_t status_b = rtc_read_register(RTC_REG_STATUS_B);
    bool is_bcd = !(status_b & RTC_STATUS_B_DM);
    bool is_24_hour = (status_b & RTC_STATUS_B_24_12) != 0;
    
    // BCD 转二进制
    if (is_bcd) {
        second = bcd_to_binary(second);
        minute = bcd_to_binary(minute);
        // 小时需要特殊处理（12 小时模式的最高位是 AM/PM）
        if (is_24_hour) {
            hour = bcd_to_binary(hour);
        } else {
            // 12 小时模式
            uint8_t pm = hour & 0x80;
            hour = bcd_to_binary(hour & 0x7F);
            if (pm && hour != 12) {
                hour += 12;
            } else if (!pm && hour == 12) {
                hour = 0;
            }
        }
        day = bcd_to_binary(day);
        month = bcd_to_binary(month);
        year = bcd_to_binary(year);
    }
    
    // 计算完整年份（假设是 20xx 年）
    uint16_t full_year = 2000 + year;
    
    // 尝试读取世纪寄存器
    uint8_t century = rtc_read_register(RTC_REG_CENTURY);
    if (century >= 19 && century <= 21) {
        full_year = century * 100 + year;
    }
    
    // 填充时间结构体
    time->second = second;
    time->minute = minute;
    time->hour = hour;
    time->weekday = weekday;
    time->day = day;
    time->month = month;
    time->year = full_year;
    
    return 0;
}

/**
 * @brief 格式化时间字符串 (HH:MM:SS)
 */
int rtc_format_time(const rtc_time_t *time, char *buf, int max_len)
{
    if (!time || !buf || max_len < 9) {
        return -1;
    }
    
    int len = snprintf(buf, max_len, "%02d:%02d:%02d",
                      time->hour, time->minute, time->second);
    
    return len;
}

/**
 * @brief 格式化日期字符串 (YYYY-MM-DD)
 */
int rtc_format_date(const rtc_time_t *time, char *buf, int max_len)
{
    if (!time || !buf || max_len < 11) {
        return -1;
    }
    
    int len = snprintf(buf, max_len, "%04d-%02d-%02d",
                      time->year, time->month, time->day);
    
    return len;
}

/**
 * @brief 获取星期名称
 */
const char *rtc_get_weekday_name(uint8_t weekday)
{
    if (weekday < 1 || weekday > 7) {
        return "Unknown";
    }
    return weekday_names[weekday - 1];
}

/**
 * @brief 获取月份名称
 */
const char *rtc_get_month_name(uint8_t month)
{
    if (month < 1 || month > 12) {
        return "Unknown";
    }
    return month_names[month - 1];
}
