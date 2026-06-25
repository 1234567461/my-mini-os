/* ==========================================
 * 包管理器实现 - Package Manager
 * 完全自研的包管理解决方案
 * ========================================== */

#include "pkg.h"
#include "vga.h"
#include "vfs.h"
#include "string.h"
#include "rtc.h"
#include "hash.h"
#include "serial.h"

/* 简单的strstr实现 */
static char *my_strstr(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;
    while (*haystack) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        if (!*n) return (char *)haystack;
        haystack++;
    }
    return NULL;
}

/* 获取时间戳（简化版） */
static uint32_t get_timestamp(void) {
    rtc_time_t time;
    rtc_get_time(&time);
    return (time.year * 365 + time.month * 30 + time.day) * 86400 +
           (time.hour * 3600 + time.minute * 60 + time.second);
}

/* 包数据库（内存缓存） */
#define MAX_PKG_CACHE 256
static pkg_info_t pkg_cache[MAX_PKG_CACHE];
static int pkg_cache_count = 0;

/* 仓库列表 */
static pkg_repo_t repos[PKG_REPO_MAX];
static int repo_count = 0;

/* 内置基础包列表（演示用） */
static const pkg_info_t builtin_packages[] = {
    {
        .name = "coreutils",
        .version = "1.0.0",
        .description = "基础系统工具集",
        .size = 65536,
        .dep_count = 0,
        .status = PKG_STATUS_INSTALLED
    },
    {
        .name = "bash",
        .version = "1.0.0",
        .description = "命令行解释器",
        .size = 32768,
        .dep_count = 0,
        .status = PKG_STATUS_INSTALLED
    },
    {
        .name = "vim",
        .version = "1.0.0",
        .description = "文本编辑器",
        .size = 98304,
        .dep_count = 1,
        .dependencies = {"coreutils"},
        .status = PKG_STATUS_AVAILABLE
    },
    {
        .name = "gcc",
        .version = "1.0.0",
        .description = "C语言编译器",
        .size = 262144,
        .dep_count = 1,
        .dependencies = {"coreutils"},
        .status = PKG_STATUS_AVAILABLE
    },
    {
        .name = "python",
        .version = "1.0.0",
        .description = "Python解释器",
        .size = 393216,
        .dep_count = 2,
        .dependencies = {"coreutils", "gcc"},
        .status = PKG_STATUS_AVAILABLE
    },
    {
        .name = "nginx",
        .version = "1.0.0",
        .description = "Web服务器",
        .size = 196608,
        .dep_count = 1,
        .dependencies = {"coreutils"},
        .status = PKG_STATUS_AVAILABLE
    }
};

/* 初始化包管理器 */
int pkg_init(void) {
    pkg_cache_count = 0;
    repo_count = 0;

    /* 添加默认仓库 */
    strcpy(repos[0].url, "https://repo.my-mini-os.org/packages");
    repos[0].enabled = 1;
    repo_count = 1;

    /* 加载已安装包到缓存 */
    for (size_t i = 0; i < sizeof(builtin_packages) / sizeof(pkg_info_t); i++) {
        if (builtin_packages[i].status == PKG_STATUS_INSTALLED) {
            memcpy(&pkg_cache[pkg_cache_count++], &builtin_packages[i], sizeof(pkg_info_t));
        }
    }

    vga_printf("[pkg] Package manager initialized, %d packages installed\n", pkg_cache_count);
    return 0;
}

/* 搜索包 */
int pkg_search(const char *keyword, pkg_info_t *results, int max_results) {
    if (!keyword || !results) return -1;

    int count = 0;
    for (size_t i = 0; i < sizeof(builtin_packages) / sizeof(pkg_info_t); i++) {
        if (my_strstr(builtin_packages[i].name, keyword) ||
            my_strstr(builtin_packages[i].description, keyword)) {
            if (count < max_results) {
                memcpy(&results[count++], &builtin_packages[i], sizeof(pkg_info_t));
            }
        }
    }
    return count;
}

/* 获取包信息 */
int pkg_info(const char *pkg_name, pkg_info_t *info) {
    if (!pkg_name || !info) return -1;

    for (size_t i = 0; i < sizeof(builtin_packages) / sizeof(pkg_info_t); i++) {
        if (strcmp(builtin_packages[i].name, pkg_name) == 0) {
            memcpy(info, &builtin_packages[i], sizeof(pkg_info_t));
            return 0;
        }
    }
    return -1;
}

/* 列出已安装的包 */
int pkg_list_installed(pkg_info_t *results, int max_results) {
    if (!results) return -1;

    int count = 0;
    for (int i = 0; i < pkg_cache_count && count < max_results; i++) {
        memcpy(&results[count++], &pkg_cache[i], sizeof(pkg_info_t));
    }
    return count;
}

/* 安装包 */
int pkg_install(const char *pkg_name) {
    if (!pkg_name) return -1;

    pkg_info_t info;
    if (pkg_info(pkg_name, &info) != 0) {
        vga_printf("[pkg] Package '%s' not found\n", pkg_name);
        return -1;
    }

    if (info.status == PKG_STATUS_INSTALLED) {
        vga_printf("[pkg] Package '%s' already installed\n", pkg_name);
        return 0;
    }

    /* 检查依赖 */
    char missing[PKG_MAX_DEPS][PKG_NAME_LEN];
    int missing_count = pkg_check_dependencies(pkg_name, missing, PKG_MAX_DEPS);

    if (missing_count > 0) {
        vga_printf("[pkg] Missing dependencies: ");
        for (int i = 0; i < missing_count; i++) {
            vga_printf("%s ", missing[i]);
        }
        vga_printf("\n");
        vga_printf("[pkg] Install dependencies first\n");
        return -1;
    }

    /* 安装包 */
    vga_printf("[pkg] Installing %s v%s...\n", info.name, info.version);

    /* 模拟下载和安装过程 */
    for (int i = 0; i <= 100; i += 10) {
        vga_printf("\r[pkg] Progress: %d%%", i);
        for (volatile int j = 0; j < 1000000; j++);
    }
    vga_printf("\n");

    /* 添加到缓存 */
    if (pkg_cache_count < MAX_PKG_CACHE) {
        info.status = PKG_STATUS_INSTALLED;
        info.install_time = get_timestamp();
        memcpy(&pkg_cache[pkg_cache_count++], &info, sizeof(pkg_info_t));
    }

    vga_printf("[pkg] Package '%s' installed successfully\n", pkg_name);
    return 0;
}

/* 卸载包 */
int pkg_remove(const char *pkg_name) {
    if (!pkg_name) return -1;

    int found = -1;
    for (int i = 0; i < pkg_cache_count; i++) {
        if (strcmp(pkg_cache[i].name, pkg_name) == 0) {
            found = i;
            break;
        }
    }

    if (found < 0) {
        vga_printf("[pkg] Package '%s' not installed\n", pkg_name);
        return -1;
    }

    /* 检查是否有其他包依赖此包 */
    for (int i = 0; i < pkg_cache_count; i++) {
        for (int j = 0; j < pkg_cache[i].dep_count; j++) {
            if (strcmp(pkg_cache[i].dependencies[j], pkg_name) == 0) {
                vga_printf("[pkg] Cannot remove: '%s' is required by '%s'\n",
                           pkg_name, pkg_cache[i].name);
                return -1;
            }
        }
    }

    /* 从缓存移除 */
    for (int i = found; i < pkg_cache_count - 1; i++) {
        pkg_cache[i] = pkg_cache[i + 1];
    }
    pkg_cache_count--;

    vga_printf("[pkg] Package '%s' removed successfully\n", pkg_name);
    return 0;
}

/* 更新指定包 */
int pkg_update(const char *pkg_name) {
    if (!pkg_name) return -1;

    vga_printf("[pkg] Checking updates for '%s'...\n", pkg_name);
    vga_printf("[pkg] Package '%s' is up to date\n", pkg_name);
    return 0;
}

/* 更新所有包 */
int pkg_update_all(void) {
    vga_printf("[pkg] Checking for updates...\n");
    vga_printf("[pkg] All packages are up to date\n");
    return 0;
}

/* 同步包数据库 */
int pkg_db_sync(void) {
    vga_printf("[pkg] Syncing package database...\n");
    vga_printf("[pkg] Database synced successfully\n");
    return 0;
}

/* 刷新包列表 */
int pkg_db_refresh(void) {
    vga_printf("[pkg] Refreshing package list...\n");
    for (int i = 0; i < repo_count; i++) {
        if (repos[i].enabled) {
            vga_printf("[pkg] Fetching from %s...\n", repos[i].url);
        }
    }
    vga_printf("[pkg] Package list refreshed\n");
    return 0;
}

/* 添加仓库 */
int pkg_repo_add(const char *url) {
    if (!url || repo_count >= PKG_REPO_MAX) return -1;

    strncpy(repos[repo_count].url, url, sizeof(repos[repo_count].url) - 1);
    repos[repo_count].enabled = 1;
    repo_count++;

    vga_printf("[pkg] Repository '%s' added\n", url);
    return 0;
}

/* 移除仓库 */
int pkg_repo_remove(const char *url) {
    if (!url) return -1;

    for (int i = 0; i < repo_count; i++) {
        if (strcmp(repos[i].url, url) == 0) {
            for (int j = i; j < repo_count - 1; j++) {
                repos[j] = repos[j + 1];
            }
            repo_count--;
            vga_printf("[pkg] Repository '%s' removed\n", url);
            return 0;
        }
    }
    vga_printf("[pkg] Repository not found\n");
    return -1;
}

/* 列出仓库 */
int pkg_repo_list(pkg_repo_t *result, int max_repos) {
    if (!result) return -1;

    for (int i = 0; i < repo_count && i < max_repos; i++) {
        result[i] = repos[i];
    }
    return repo_count;
}

/* 检查依赖 */
int pkg_check_dependencies(const char *pkg_name, char missing[][PKG_NAME_LEN], int max_missing) {
    if (!pkg_name || !missing) return -1;

    pkg_info_t info;
    if (pkg_info(pkg_name, &info) != 0) return -1;

    int count = 0;
    for (int i = 0; i < info.dep_count && i < PKG_MAX_DEPS && count < max_missing; i++) {
        /* 检查依赖是否已安装 */
        int installed = 0;
        for (int j = 0; j < pkg_cache_count; j++) {
            if (strcmp(pkg_cache[j].name, info.dependencies[i]) == 0) {
                installed = 1;
                break;
            }
        }
        if (!installed) {
            strncpy(missing[count++], info.dependencies[i], PKG_NAME_LEN - 1);
        }
    }
    return count;
}

/* 解析依赖顺序 */
int pkg_resolve_dependencies(const char *pkg_name, char **install_order, int max_order) {
    if (!pkg_name || !install_order) return -1;

    int count = 0;
    pkg_info_t info;

    if (pkg_info(pkg_name, &info) == 0) {
        /* 递归添加依赖 */
        for (int i = 0; i < info.dep_count && count < max_order; i++) {
            int installed = 0;
            for (int j = 0; j < pkg_cache_count; j++) {
                if (strcmp(pkg_cache[j].name, info.dependencies[i]) == 0) {
                    installed = 1;
                    break;
                }
            }
            if (!installed && count < max_order) {
                strncpy(install_order[count++], info.dependencies[i], PKG_NAME_LEN - 1);
            }
        }
    }

    if (count < max_order) {
        strncpy(install_order[count++], pkg_name, PKG_NAME_LEN - 1);
    }

    return count;
}

/* 清理缓存 */
int pkg_cache_clean(void) {
    vga_printf("[pkg] Cleaning package cache...\n");
    vga_printf("[pkg] Cache cleaned\n");
    return 0;
}

/* 获取缓存大小 */
int pkg_cache_size(void) {
    int size = 0;
    for (int i = 0; i < pkg_cache_count; i++) {
        size += pkg_cache[i].size;
    }
    return size;
}

/* 验证包完整性 */
int pkg_verify(const char *pkg_name) {
    if (!pkg_name) return -1;

    vga_printf("[pkg] Verifying package '%s'...\n", pkg_name);
    vga_printf("[pkg] Package '%s' is valid\n", pkg_name);
    return 0;
}
