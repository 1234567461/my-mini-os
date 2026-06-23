/* ==========================================
 * 用户权限管理实现 v0.5.0
 * 功能：
 *   1. 当前用户状态管理
 *   2. 权限检查
 *   3. su 切换用户（密码验证）
 *   4. 登录/登出
 * ========================================== */

#include "user.h"
#include "string.h"

/* ==========================================
 * 内置用户账户（简化版）
 * 实际系统中应该从文件系统读取
 * ========================================== */
#define MAX_USERS  4

typedef struct {
    char username[32];
    char password[32];
    user_level_t level;
} user_account_t;

/* 内置用户列表 */
static user_account_t user_accounts[MAX_USERS] = {
    {"root",     "toor",     USER_ADMIN},   /* 管理员，密码：toor */
    {"user",     "123456",   USER_NORMAL},  /* 普通用户，密码：123456 */
    {"guest",    "",         USER_GUEST},   /* 访客，无密码 */
    {"kernel",   "",         USER_KERNEL},  /* 内核用户（特殊） */
};

/* ==========================================
 * 当前用户状态
 * ========================================== */
static user_t current_user;

/* ==========================================
 * 初始化用户系统
 * ========================================== */
void user_init() {
    /* 默认以普通用户登录 */
    strncpy(current_user.username, "user", 31);
    current_user.username[31] = '\0';
    current_user.level = USER_NORMAL;
    current_user.logged_in = true;
}

/* ==========================================
 * 获取当前用户
 * ========================================== */
user_t *user_get_current() {
    return &current_user;
}

/* ==========================================
 * 获取当前用户名
 * ========================================== */
const char *user_get_name() {
    return current_user.username;
}

/* ==========================================
 * 获取当前权限级别
 * ========================================== */
user_level_t user_get_level() {
    return current_user.level;
}

/* ==========================================
 * 检查是否有指定级别的权限
 * ========================================== */
bool user_has_permission(user_level_t required_level) {
    return current_user.level >= required_level;
}

/* ==========================================
 * 检查是否是管理员
 * ========================================== */
bool user_is_admin() {
    return current_user.level >= USER_ADMIN;
}

/* ==========================================
 * 查找用户账户
 * ========================================== */
static user_account_t *find_account(const char *username) {
    if (username == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < MAX_USERS; i++) {
        if (strcmp(user_accounts[i].username, username) == 0) {
            return &user_accounts[i];
        }
    }
    
    return NULL;
}

/* ==========================================
 * 切换用户（su命令）
 * 返回：0=成功，-1=用户不存在，-2=密码错误
 * ========================================== */
int user_su(const char *username, const char *password) {
    user_account_t *account = find_account(username);
    
    if (account == NULL) {
        return -1;  /* 用户不存在 */
    }
    
    /* 验证密码 */
    if (password == NULL) {
        password = "";
    }
    
    if (strcmp(account->password, password) != 0) {
        return -2;  /* 密码错误 */
    }
    
    /* 切换用户 */
    strncpy(current_user.username, account->username, 31);
    current_user.username[31] = '\0';
    current_user.level = account->level;
    current_user.logged_in = true;
    
    return 0;  /* 成功 */
}

/* ==========================================
 * 登录
 * ========================================== */
int user_login(const char *username, const char *password) {
    return user_su(username, password);
}

/* ==========================================
 * 登出
 * ========================================== */
void user_logout() {
    strncpy(current_user.username, "guest", 31);
    current_user.username[31] = '\0';
    current_user.level = USER_GUEST;
    current_user.logged_in = false;
}

/* ==========================================
 * 获取权限级别名称
 * ========================================== */
const char *user_level_name(user_level_t level) {
    switch (level) {
        case USER_GUEST:
            return "guest";
        case USER_NORMAL:
            return "user";
        case USER_ADMIN:
            return "admin";
        case USER_KERNEL:
            return "kernel";
        default:
            return "unknown";
    }
}
