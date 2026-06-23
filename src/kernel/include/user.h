/* ==========================================
 * 用户权限管理 - user.h v0.5.0
 * 功能：
 *   1. 用户权限级别定义
 *   2. 当前用户状态管理
 *   3. 权限检查函数
 *   4. su / whoami 命令支持
 * ========================================== */

#ifndef USER_H
#define USER_H

#include "types.h"

/* ==========================================
 * 用户权限级别
 * ========================================== */
typedef enum {
    USER_GUEST      = 0,    /* 访客（最低权限） */
    USER_NORMAL     = 1,    /* 普通用户（默认） */
    USER_ADMIN      = 2,    /* 管理员（root） */
    USER_KERNEL     = 3     /* 内核态（最高权限） */
} user_level_t;

/* ==========================================
 * 用户信息结构体
 * ========================================== */
typedef struct {
    char username[32];      /* 用户名 */
    user_level_t level;     /* 权限级别 */
    bool logged_in;         /* 是否已登录 */
} user_t;

/* ==========================================
 * 函数声明
 * ========================================== */

/* 初始化用户系统 */
void user_init();

/* 获取当前用户 */
user_t *user_get_current();

/* 获取当前用户名 */
const char *user_get_name();

/* 获取当前权限级别 */
user_level_t user_get_level();

/* 检查是否有指定级别的权限 */
bool user_has_permission(user_level_t required_level);

/* 检查是否是管理员 */
bool user_is_admin();

/* 切换用户（su命令） */
int user_su(const char *username, const char *password);

/* 登录 */
int user_login(const char *username, const char *password);

/* 登出 */
void user_logout();

/* 获取权限级别名称 */
const char *user_level_name(user_level_t level);

#endif /* USER_H */
