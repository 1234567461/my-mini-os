/* ==========================================
 * 包管理器 - Package Manager
 * 完全自研的包管理解决方案
 * ========================================== */

#ifndef PKG_H
#define PKG_H

#include "types.h"

/* 包信息 */
#define PKG_NAME_LEN    32
#define PKG_VERSION_LEN 16
#define PKG_DESC_LEN    128
#define PKG_MAX_DEPS    8

typedef enum {
    PKG_STATUS_NONE = 0,
    PKG_STATUS_INSTALLED,
    PKG_STATUS_AVAILABLE,
    PKG_STATUS_UPDATE_AVAIL,
    PKG_STATUS_BROKEN
} pkg_status_t;

typedef struct {
    char name[PKG_NAME_LEN];
    char version[PKG_VERSION_LEN];
    char description[PKG_DESC_LEN];
    uint32_t size;              /* 包大小（字节） */
    uint32_t install_time;      /* 安装时间戳 */
    char dependencies[PKG_MAX_DEPS][PKG_NAME_LEN];
    int dep_count;
    pkg_status_t status;
} pkg_info_t;

/* 包数据库 */
#define PKG_DB_PATH     "/etc/pkg.db"
#define PKG_DB_MAGIC    0x504B4731  /* "PKG1" */
#define PKG_DB_VERSION  1

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t pkg_count;
    uint32_t checksum;
} pkg_db_header_t;

/* 仓库配置 */
#define PKG_REPO_MAX    4
typedef struct {
    char url[128];
    int enabled;
} pkg_repo_t;

/* ==========================================
 * 包管理器操作
 * ========================================== */

/* 初始化包管理器 */
int pkg_init(void);

/* 包查询 */
int pkg_search(const char *keyword, pkg_info_t *results, int max_results);
int pkg_info(const char *pkg_name, pkg_info_t *info);
int pkg_list_installed(pkg_info_t *results, int max_results);

/* 包安装/卸载 */
int pkg_install(const char *pkg_name);
int pkg_remove(const char *pkg_name);
int pkg_update(const char *pkg_name);
int pkg_update_all(void);

/* 包数据库 */
int pkg_db_sync(void);           /* 同步本地数据库 */
int pkg_db_refresh(void);        /* 从仓库刷新包列表 */

/* 仓库管理 */
int pkg_repo_add(const char *url);
int pkg_repo_remove(const char *url);
int pkg_repo_list(pkg_repo_t *repos, int max_repos);

/* 依赖解析 */
int pkg_check_dependencies(const char *pkg_name, char missing[][PKG_NAME_LEN], int max_missing);
int pkg_resolve_dependencies(const char *pkg_name, char **install_order, int max_order);

/* 缓存管理 */
int pkg_cache_clean(void);
int pkg_cache_size(void);

/* 验证包完整性 */
int pkg_verify(const char *pkg_name);

#endif /* PKG_H */
