/* ==========================================
 * version.c - 版本管理实现
 * ========================================== */

#include "version.h"
#include "vga.h"
#include "rtc.h"
#include "vfs.h"
#include "klog.h"
#include "string.h"

/* 版本信息存储地址（使用固定内存地址） */
#define VERSION_STORAGE_ADDR 0x90000
#define VERSION_MAGIC 0x56455253  /* "VERS" */

typedef struct {
    uint32_t magic;
    version_info_t info;
} version_storage_t;

/* 当前版本信息 */
static version_info_t current_version = {
    .version = "v1.1.0",
    .build_date = "2026-06-25",
    .last_check = "2026-06-25",
    .build_number = 20260625
};

/* 默认最新版本信息（模拟，实际应该从网络获取） */
static const char *default_latest_version = "v1.1.0";

/* 简单的格式化日期字符串函数 */
static void format_date(int year, int month, int day, char *buf, int size) {
    int i = 0;
    /* 年 */
    if (year >= 1000) buf[i++] = '0' + year / 1000;
    if (year >= 100) buf[i++] = '0' + (year / 100) % 10;
    if (year >= 10) buf[i++] = '0' + (year / 10) % 10;
    buf[i++] = '0' + year % 10;
    buf[i++] = '-';
    /* 月 */
    buf[i++] = '0' + month / 10;
    buf[i++] = '0' + month % 10;
    buf[i++] = '-';
    /* 日 */
    buf[i++] = '0' + day / 10;
    buf[i++] = '0' + day % 10;
    buf[i] = '\0';
}

/* 初始化版本系统 */
void version_init(void) {
    /* 从存储读取版本信息 */
    version_storage_t *storage = (version_storage_t *)VERSION_STORAGE_ADDR;
    
    if (storage->magic == VERSION_MAGIC) {
        /* 使用存储的版本信息 */
        memcpy(&current_version, &storage->info, sizeof(version_info_t));
        klog_log("version", "Loaded version info from storage");
    } else {
        /* 使用默认值 */
        klog_log("version", "Using default version info");
        
        /* 获取当前日期 */
        rtc_time_t time;
        rtc_get_time(&time);

        format_date(time.year, time.month, time.day, current_version.last_check, VERSION_DATE_LEN);

        /* 保存到存储 */
        storage->magic = VERSION_MAGIC;
        memcpy(&storage->info, &current_version, sizeof(version_info_t));
    }
    
    klog_logf("version", "Current version: %s, built: %s", 
              current_version.version, current_version.build_date);
}

/* 获取当前版本信息 */
version_info_t* version_get_current(void) {
    return &current_version;
}

/* 解析版本号中的数字 */
static int version_parse_component(const char *version, int *major, int *minor, int *patch) {
    /* 跳过v前缀 */
    if (version[0] == 'v' || version[0] == 'V') version++;

    /* 手动解析版本号 */
    int m = 0, n = 0, p = 0;
    int digit_count = 0;
    int part = 0;

    while (*version && part < 3) {
        if (*version >= '0' && *version <= '9') {
            if (part == 0) m = m * 10 + (*version - '0');
            else if (part == 1) n = n * 10 + (*version - '0');
            else p = p * 10 + (*version - '0');
        } else if (*version == '.') {
            part++;
        }
        version++;
    }

    *major = m;
    *minor = n;
    *patch = p;
    return 0;
}

/* 比较版本号：返回1表示v1>v2, 0表示相等, -1表示v1<v2 */
int version_compare(const char *v1, const char *v2) {
    int maj1, min1, pat1;
    int maj2, min2, pat2;
    
    if (version_parse_component(v1, &maj1, &min1, &pat1) != 0) return -1;
    if (version_parse_component(v2, &maj2, &min2, &pat2) != 0) return 1;
    
    if (maj1 > maj2) return 1;
    if (maj1 < maj2) return -1;
    
    if (min1 > min2) return 1;
    if (min1 < min2) return -1;
    
    if (pat1 > pat2) return 1;
    if (pat1 < pat2) return -1;
    
    return 0;
}

/* 解析日期字符串 */
static int parse_date(const char *date_str, int *year, int *month, int *day) {
    /* 手动解析 YYYY-MM-DD 格式 */
    int y = 0, m = 0, d = 0;
    int part = 0;
    const char *p = date_str;

    while (*p) {
        if (*p >= '0' && *p <= '9') {
            if (part == 0) y = y * 10 + (*p - '0');
            else if (part == 1) m = m * 10 + (*p - '0');
            else d = d * 10 + (*p - '0');
        } else if (*p == '-') {
            part++;
        }
        p++;
    }

    *year = y;
    *month = m;
    *day = d;
    return (y > 0 && m > 0 && d > 0) ? 0 : -1;
}

/* 计算两个日期之间的天数 */
static int days_between(const char *date1, const char *date2) {
    int y1, m1, d1;
    int y2, m2, d2;
    
    if (parse_date(date1, &y1, &m1, &d1) != 0) return 0;
    if (parse_date(date2, &y2, &m2, &d2) != 0) return 0;
    
    /* 简化的天数计算（不考虑闰年） */
    int days1 = y1 * 365 + m1 * 30 + d1;
    int days2 = y2 * 365 + m2 * 30 + d2;
    
    return days2 - days1;
}

/* 获取距上次更新的天数 */
int version_days_since_update(void) {
    rtc_time_t time;
    rtc_get_time(&time);

    char today[VERSION_DATE_LEN];
    format_date(time.year, time.month, time.day, today, VERSION_DATE_LEN);

    return days_between(current_version.build_date, today);
}

/* 检查是否超过3个月未更新（约90天） */
int version_is_stale(void) {
    return version_days_since_update() > 90;
}

/* 获取最新版本信息 */
version_status_t version_check_latest(char *latest_version, size_t buf_size) {
    /* 模拟从网络获取最新版本（实际需要HTTP客户端） */
    if (buf_size > 0) {
        strncpy(latest_version, default_latest_version, buf_size - 1);
        latest_version[buf_size - 1] = '\0';
    }
    
    /* 比较版本 */
    int cmp = version_compare(current_version.version, default_latest_version);
    
    if (cmp < 0) {
        return VERSION_UPDATE_AVAILABLE;
    } else if (cmp == 0) {
        return VERSION_OK;
    } else {
        /* 当前版本比最新版本还新（开发版本） */
        return VERSION_OK;
    }
}

/* 更新最后检查时间 */
void version_update_check_time(void) {
    rtc_time_t time;
    rtc_get_time(&time);

    format_date(time.year, time.month, time.day, current_version.last_check, VERSION_DATE_LEN);

    /* 保存到存储 */
    version_storage_t *storage = (version_storage_t *)VERSION_STORAGE_ADDR;
    storage->magic = VERSION_MAGIC;
    memcpy(&storage->info, &current_version, sizeof(version_info_t));
}

/* 显示版本信息 */
void version_print_info(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("=== Version Information ===\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    vga_printf("Current Version: %s\n", current_version.version);
    vga_printf("Build Date:      %s\n", current_version.build_date);
    vga_printf("Last Check:      %s\n", current_version.last_check);
    vga_printf("Days Since Build: %d\n", version_days_since_update());
    vga_printf("Build Number:    %u\n", current_version.build_number);
}

/* 显示更新状态 */
void version_print_update_status(void) {
    char latest[VERSION_MAX_LEN];
    version_status_t status = version_check_latest(latest, sizeof(latest));
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("=== Update Status ===\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    vga_printf("Current:  %s\n", current_version.version);
    vga_printf("Latest:   %s\n", latest);
    
    int days = version_days_since_update();
    vga_printf("Days since last build: %d\n", days);
    
    if (status == VERSION_UPDATE_AVAILABLE) {
        vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
        vga_puts("\n⚠️  Update available! Run 'update upgrade' to upgrade.\n");
    } else if (days > 90) {
        vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
        vga_puts("\n⚠️  WARNING: Your system has not been updated in over 3 months!\n");
        vga_puts("    Please run 'update upgrade' to get the latest version.\n");
    } else {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_puts("\n✓ System is up to date!\n");
    }
    
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    /* 更新检查时间 */
    version_update_check_time();
}
