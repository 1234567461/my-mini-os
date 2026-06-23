/* ==========================================
 * 字符串操作函数实现
 * ========================================== */

#include "string.h"

/* 字符串长度 */
size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

/* 字符串复制 */
char *strcpy(char *dst, const char *src) {
    char *ret = dst;
    while ((*dst++ = *src++));
    return ret;
}

/* 字符串复制（指定长度） */
char *strncpy(char *dst, const char *src, size_t n) {
    char *ret = dst;
    size_t i;
    for (i = 0; i < n && src[i]; i++) {
        dst[i] = src[i];
    }
    for (; i < n; i++) {
        dst[i] = '\0';
    }
    return ret;
}

/* 字符串连接 */
char *strcat(char *dst, const char *src) {
    char *ret = dst;
    while (*dst) dst++;
    while ((*dst++ = *src++));
    return ret;
}

/* 字符串比较 */
int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

/* 字符串比较（指定长度） */
int strncmp(const char *s1, const char *s2, size_t n) {
    size_t i;
    for (i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return s1[i] - s2[i];
        }
        if (s1[i] == '\0') {
            break;
        }
    }
    return 0;
}

/* 字符查找 */
char *strchr(const char *str, char c) {
    while (*str) {
        if (*str == c) {
            return (char *)str;
        }
        str++;
    }
    if (c == '\0') {
        return (char *)str;
    }
    return NULL;
}

/* 字符串转整数 */
int atoi(const char *str) {
    int result = 0;
    int sign = 1;

    /* 跳过空白字符 */
    while (*str == ' ' || *str == '\t') {
        str++;
    }

    /* 处理符号 */
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    /* 转换数字 */
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }

    return result * sign;
}

/* 整数转字符串 */
char *itoa(int value, char *str, int base) {
    char *ret = str;
    char *ptr = str;
    bool negative = false;

    /* 只支持2-36进制 */
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }

    /* 处理负数（只对十进制） */
    if (value < 0 && base == 10) {
        negative = true;
        value = -value;
    }

    /* 转换数字（逆序） */
    do {
        int digit = value % base;
        if (digit < 10) {
            *ptr++ = '0' + digit;
        } else {
            *ptr++ = 'a' + digit - 10;
        }
        value /= base;
    } while (value > 0);

    /* 添加负号 */
    if (negative) {
        *ptr++ = '-';
    }

    *ptr = '\0';

    /* 反转字符串 */
    char *start = str;
    char *end = ptr - 1;
    while (start < end) {
        char tmp = *start;
        *start = *end;
        *end = tmp;
        start++;
        end--;
    }

    return ret;
}
