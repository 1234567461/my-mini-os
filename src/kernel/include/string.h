/* ==========================================
 * 字符串操作函数
 * ========================================== */

#ifndef STRING_H
#define STRING_H

#include "types.h"

/* 字符串长度 */
size_t strlen(const char *str);

/* 字符串复制 */
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t n);

/* 字符串连接 */
char *strcat(char *dst, const char *src);

/* 字符串比较 */
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

/* 字符查找 */
char *strchr(const char *str, char c);

/* 字符串转数字 */
int atoi(const char *str);

/* 数字转字符串 */
char *itoa(int value, char *str, int base);

/* 内存操作 */
void *memset(void *ptr, int value, size_t num);
void *memcpy(void *dest, const void *src, size_t num);
int memcmp(const void *ptr1, const void *ptr2, size_t num);

#endif /* STRING_H */
