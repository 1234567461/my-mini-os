/* ==========================================
 * 实时时钟（RTC）驱动实现 - rtc.c
 * 功能：
 *   1. CMOS RTC 驱动
 *   2. 读取当前时间（年、月、日、时、分、秒）
 *   3. BCD 转换
 * ========================================== */
#include "rtc.h"
#include "types.h"
#include "vga.h"
#include "string.h"

/* RTC 是否使用 BCD 模式 */
static bool rtc_bcd_mode = true;
static bool rtc_24hour_mode = true;

/* ==========================================
 * 函数：rtc_read_register
 * 功能：读取 RTC 寄存器
 * ========================================== */
uint8_t rtc_read_register(uint8_t reg)
{
    outb(RTC_PORT_INDEX, reg);
    return inb(RTC_PORT_DATA);
}

/* ==========================================
 * 函数：rtc_write_register
 * 功能：写入 RTC 寄存器
 * ========================================== */
void rtc_write_register(uint8_t reg, uint8_t value)
{
    outb(RTC_PORT_INDEX, reg);
    outb(RTC_PORT_DATA, value);
}

/* ==========================================
 * 函数：rtc_is_updating
 * 功能：检查 RTC 是否正在更新
 * ========================================== */
bool rtc_is_updating(void)
{
    return (rtc_read_register(RTC_REG_STATUS_A) & RTC_STATUS_A_UIP) != 0;
}

/* ==========================================
 * 函数：rtc_bcd_to_binary
 * 功能：BCD 码转二进制
 * ========================================== */
uint8_t rtc_bcd_to_binary(uint8_t bcd)
{
    return (bcd & 0x0F) + ((bcd >> 4) & 0x0F) * 10;
}

/* ==========================================
 * 函数：rtc_binary_to_bcd
 * 功能：二进制转 BCD 码
 * ========================================== */
uint8_t rtc_binary_to_bcd(uint8_t binary)
{
    return ((binary / 10) << 4) | (binary % 10);
}

/* ==========================================
 * 函数：rtc_get_time
 * 功能：读取当前时间
 * ========================================== */
void rtc_get_time(rtc_time_t *time)
{
    if (time == NULL) {
        return;
    }

    /* 等待更新完成 */
    while (rtc_is_updating()) {
        /* 忙等待 */
    }

    /* 读取时间寄存器 */
    uint8_t second = rtc_read_register(RTC_REG_SECONDS);
    uint8_t minute = rtc_read_register(RTC_REG_MINUTES);
    uint8_t hour = rtc_read_register(RTC_REG_HOURS);
    uint8_t day = rtc_read_register(RTC_REG_DAY);
    uint8_t month = rtc_read_register(RTC_REG_MONTH);
    uint8_t year = rtc_read_register(RTC_REG_YEAR);
    uint8_t weekday = rtc_read_register(RTC_REG_WEEKDAY);
    uint8_t century = rtc_read_register(RTC_REG_CENTURY);

    /* 如果是 BCD 模式，转换为二进制 */
    if (rtc_bcd_mode) {
        second = rtc_bcd_to_binary(second);
        minute = rtc_bcd_to_binary(minute);
        hour = rtc_bcd_to_binary(hour & 0x7F);  /* 清除最高位（AM/PM） */
        day = rtc_bcd_to_binary(day);
        month = rtc_bcd_to_binary(month);
        year = rtc_bcd_to_binary(year);
        century = rtc_bcd_to_binary(century);
    }

    /* 处理 12 小时制 */
    if (!rtc_24hour_mode && (hour & 0x80)) {
        /* PM */
        hour = (hour & 0x7F) + 12;
        if (hour == 24) {
            hour = 12;  /* 12 PM = 12:00 */
        }
    }

    /* 填充时间结构体 */
    time->second = second;
    time->minute = minute;
    time->hour = hour;
    time->day = day;
    time->month = month;
    time->year = century * 100 + year;
    time->weekday = weekday;

    /* 如果年份小于 100，假设是 2000 年以后 */
    if (time->year < 100) {
        time->year += 2000;
    }
}

/* ==========================================
 * 函数：rtc_format_time
 * 功能：格式化时间字符串 (HH:MM:SS)
 * ========================================== */
void rtc_format_time(rtc_time_t *time, char *buf, int buf_size)
{
    if (time == NULL || buf == NULL || buf_size < 9) {
        return;
    }

    buf[0] = '0' + time->hour / 10;
    buf[1] = '0' + time->hour % 10;
    buf[2] = ':';
    buf[3] = '0' + time->minute / 10;
    buf[4] = '0' + time->minute % 10;
    buf[5] = ':';
    buf[6] = '0' + time->second / 10;
    buf[7] = '0' + time->second % 10;
    buf[8] = '\0';
}

/* ==========================================
 * 函数：rtc_format_date
 * 功能：格式化日期字符串 (YYYY-MM-DD)
 * ========================================== */
void rtc_format_date(rtc_time_t *time, char *buf, int buf_size)
{
    if (time == NULL || buf == NULL || buf_size < 11) {
        return;
    }

    /* 年份 */
    int year = time->year;
    buf[0] = '0' + (year / 1000) % 10;
    buf[1] = '0' + (year / 100) % 10;
    buf[2] = '0' + (year / 10) % 10;
    buf[3] = '0' + year % 10;
    buf[4] = '-';
    
    /* 月份 */
    buf[5] = '0' + time->month / 10;
    buf[6] = '0' + time->month % 10;
    buf[7] = '-';
    
    /* 日期 */
    buf[8] = '0' + time->day / 10;
    buf[9] = '0' + time->day % 10;
    buf[10] = '\0';
}

/* ==========================================
 * 函数：rtc_init
 * 功能：初始化 RTC
 * ========================================== */
void rtc_init(void)
{
    /* 读取状态寄存器 B，检查模式 */
    uint8_t status_b = rtc_read_register(RTC_REG_STATUS_B);
    
    rtc_bcd_mode = !(status_b & RTC_STATUS_B_BINARY);
    rtc_24hour_mode = (status_b & RTC_STATUS_B_24H) != 0;

    /* 禁用 RTC 中断（我们暂时不用中断模式） */
    rtc_write_register(RTC_REG_STATUS_B, status_b & ~(RTC_STATUS_B_PIE | RTC_STATUS_B_AIE | RTC_STATUS_B_UIE));

    /* 读取一次时间验证 */
    rtc_time_t time;
    rtc_get_time(&time);

    vga_puts("  [✓] RTC Real-Time Clock\n");
}
