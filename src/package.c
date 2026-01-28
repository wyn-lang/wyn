// Complete package manager implementation for Wyn
// Supports local registry, version resolution, and dependency management

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>

#ifdef _WIN32
    #include <direct.h>
    #include <io.h>
    #define mkdir(path, mode) _mkdir(path)
#else
    #define _POSIX_C_SOURCE 200809L
    #include <dirent.h>
#endif

#include "package.h"
#include "toml.h"

// Package registry structure
typedef struct {
    char name[128];
    char version[32];
    char path[512];
    int dependency_count;
    char dependencies[32][128];  // name@version format
} PackageInfo;

typedef struct {
    PackageInfo packages[256];
    int count;
} PackageRegistry;

static PackageRegistry global_registry = {0};

// Utility functions
static char* get_wyn_home() {
    static char wyn_home[1024];
    char* home = getenv("HOME");
    if (!home) home = getenv("USERPROFILE");
    if (!home) return NULL;
    
    snprintf(wyn_home, sizeof(wyn_home), "%s/.wyn", home);
    return wyn_home;
}

static char* get_package_dir() {
    static char pkg_dir[1024];
    char* wyn_home = get_wyn_home();
    if (!wyn_home) return NULL;
    
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/packages", wyn_home);
    return pkg_dir;
}

static char* get_registry_file() {
    static char registry_file[1024];
    char* wyn_home = get_wyn_home();
    if (!wyn_home) return NULL;
    
    snprintf(registry_file, sizeof(registry_file), "%s/registry.txt", wyn_home);
    return registry_file;
}

static int ensure_directories() {
    char* wyn_home = get_wyn_home();
    if (!wyn_home) return -1;
    
    mkdir(wyn_home, 0755);
    
    char pkg_dir[1024];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/packages", wyn_home);
    mkdir(pkg_dir, 0755);
    
    return 0;
}

// Version comparison (simple semver)
static int compare_versions(const char* v1, const char* v2) {
    int major1, minor1, patch1;
    int major2, minor2, patch2;
    
    sscanf(v1, "%d.%d.%d", &major1, &minor1, &patch1);
    sscanf(v2, "%d.%d.%d", &major2, &minor2, &patch2);
    
    if (major1 != major2) return major1 - major2;
    if (minor1 != minor2) return minor1 - minor2;
    return patch1 - patch2;
}

static bool version_satisfies(const char* version, const char* requirement) {
    // Simple version matching
    // Supports: "1.0.0", "^1.0.0" (compatible), "~1.0.0" (patch), ">=1.0.0"
    
    if (requirement[0] == '^') {
        // Compatible version (same major)
        int req_major, ver_major;
        sscanf(requirement + 1, "%d", &req_major);
        sscanf(version, "%d", &ver_major);
        return ver_major == req_major && compare_versions(version, requirement + 1) >= 0;
    }
    
    if (requirement[0] == '~') {
        // Patch version (same major.minor)
        int req_major, req_minor, ver_major, ver_minor;
        sscanf(requirement + 1, "%d.%d", &req_major, &req_minor);
        sscanf(version, "%d.%d", &ver_major, &ver_minor);
        return ver_major == req_major && ver_minor == req_minor;
    }
    
    if (strncmp(requirement, ">=", 2) == 0) {
        return compare_versions(version, requirement + 2) >= 0;
    }
    
    if (strncmp(requirement, "<=", 2) == 0) {
        return compare_versions(version, requirement + 2) <= 0;
    }
    
    if (requirement[0] == '>') {
        return compare_versions(version, requirement + 1) > 0;
    }
    
    if (requirement[0] == '<') {
        return compare_versions(version, requirement + 1) < 0;
    }
    
    // Exact match
    return strcmp(version, requirement) == 0;
}

// Registry operations
static int load_registry() {
    char* registry_file = get_registry_file();
    if (!registry_file) return -1;
    
    FILE* f = fopen(registry_file, "r");
    if (!f) {
        // No registry yet, that's okay
        global_registry.count = 0;
        return 0;
    }
    
    global_registry.count = 0;
    char line[1024];
    
    while (fgets(line, sizeof(line), f) && global_registry.count < 256) {
        PackageInfo* pkg = &global_registry.packages[global_registry.count];
        
        // Format: name|version|path
        char* name = strtok(line, "|");
        char* version = strtok(NULL, "|");
        char* path = strtok(NULL, "|\n");
        
        if (name && version && path) {
            strncpy(pkg->name, name, sizeof(pkg->name) - 1);
            strncpy(pkg->version, version, sizeof(pkg->version) - 1);
            strncpy(pkg->path, path, sizeof(pkg->path) - 1);
            global_registry.count++;
        }
    }
    
    fclose(f);
    return 0;
}

static int save_registry() {
    char* registry_file = get_registry_file();
    if (!registry_file) return -1;
    
    FILE* f = fopen(registry_file, "w");
    if (!f) return -1;
    
    for (int i = 0; i < global_registry.count; i++) {
        PackageInfo* pkg = &global_registry.packages[i];
        fprintf(f, "%s|%s|%s\n", pkg->name, pkg->version, pkg->path);
    }
    
    fclose(f);
    return 0;
}

static PackageInfo* find_package(const char* name, const char* version_req) {
    for (int i = 0; i < global_registry.count; i++) {
        PackageInfo* pkg = &global_registry.packages[i];
        if (strcmp(pkg->name, name) == 0) {
            if (!version_req || version_satisfies(pkg->version, version_req)) {
                return pkg;
            }
        }
    }
    return NULL;
}

static int register_package(const char* name, const char* version, const char* path) {
    if (global_registry.count >= 256) return -1;
    
    // Check if already registered
    for (int i = 0; i < global_registry.count; i++) {
        PackageInfo* pkg = &global_registry.packages[i];
        if (strcmp(pkg->name, name) == 0 && strcmp(pkg->version, version) == 0) {
            // Update path
            strncpy(pkg->path, path, sizeof(pkg->path) - 1);
            return save_registry();
        }
    }
    
    // Add new package
    PackageInfo* pkg = &global_registry.packages[global_registry.count];
    strncpy(pkg->name, name, sizeof(pkg->name) - 1);
    strncpy(pkg->version, version, sizeof(pkg->version) - 1);
    strncpy(pkg->path, path, sizeof(pkg->path) - 1);
    pkg->dependency_count = 0;
    global_registry.count++;
    
    return save_registry();
}

// Package installation
static int install_from_git(const char* name, const char* url, const char* install_path) {
    printf("  Cloning %s from %s...\n", name, url);
    
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "git clone --quiet %s %s 2>&1", url, install_path);
    
    int result = system(cmd);
    if (result != 0) {
        fprintf(stderr, "  ✗ Failed to clone repository\n");
        return -1;
    }
    
    return 0;
}

static int install_from_local(const char* name, const char* source, const char* install_path) {
    printf("  Copying %s from %s...\n", name, source);
    
    char cmd[2048];
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "xcopy /E /I /Y /Q \"%s\" \"%s\" >nul 2>&1", source, install_path);
#else
    snprintf(cmd, sizeof(cmd), "cp -r \"%s\" \"%s\" 2>&1", source, install_path);
#endif
    
    int result = system(cmd);
    if (result != 0) {
        fprintf(stderr, "  ✗ Failed to copy package\n");
        return -1;
    }
    
    return 0;
}

static int install_package_internal(const char* name, const char* version_or_source, bool is_dependency) {
    char* pkg_dir = get_package_dir();
    if (!pkg_dir) return -1;
    
    // Check if already installed
    PackageInfo* existing = find_package(name, version_or_source);
    if (existing) {
        if (!is_dependency) {
            printf("  %s@%s - already installed\n", name, existing->version);
        }
        return 0;
    }
    
    // Determine install path
    char install_path[1024];
    snprintf(install_path, sizeof(install_path), "%s/%s", pkg_dir, name);
    
    // Check if path already exists
    struct stat st;
    if (stat(install_path, &st) == 0) {
        // Already exists, just register it
        printf("  %s - found at %s\n", name, install_path);
        return register_package(name, version_or_source, install_path);
    }
    
    // Install based on source type
    int result = -1;
    
    if (strstr(version_or_source, "://")) {
        // Git URL
        result = install_from_git(name, version_or_source, install_path);
    } else if (stat(version_or_source, &st) == 0) {
        // Local path
        result = install_from_local(name, version_or_source, install_path);
    } else {
        fprintf(stderr, "  ✗ Unknown source type for %s: %s\n", name, version_or_source);
        return -1;
    }
    
    if (result == 0) {
        // Register package
        register_package(name, version_or_source, install_path);
        printf("  ✓ %s installed\n", name);
        
        // Check for dependencies in installed package
        char dep_toml[1024];
        snprintf(dep_toml, sizeof(dep_toml), "%s/wyn.toml", install_path);
        
        WynConfig* config = wyn_config_parse(dep_toml);
        if (config && config->dependency_count > 0) {
            printf("  Installing %d dependencies for %s...\n", config->dependency_count, name);
            for (int i = 0; i < config->dependency_count; i++) {
                install_package_internal(config->dependencies[i].name,
                                       config->dependencies[i].version,
                                       true);
            }
            wyn_config_free(config);
        }
    }
    
    return result;
}

// Public API
int package_install(const char* project_dir) {
    printf("Installing packages for %s...\n", project_dir);
    
    if (ensure_directories() != 0) {
        fprintf(stderr, "Error: Could not create package directories\n");
        return 1;
    }
    
    load_registry();
    
    // Read wyn.toml
    char toml_path[1024];
    snprintf(toml_path, sizeof(toml_path), "%s/wyn.toml", project_dir);
    
    WynConfig* config = wyn_config_parse(toml_path);
    if (!config) {
        fprintf(stderr, "Error: Could not read wyn.toml\n");
        return 1;
    }
    
    printf("Project: %s v%s\n",
           config->project.name ? config->project.name : "unknown",
           config->project.version ? config->project.version : "0.0.0");
    
    if (config->dependency_count == 0) {
        printf("No dependencies to install.\n");
        wyn_config_free(config);
        return 0;
    }
    
    printf("Installing %d dependencies...\n\n", config->dependency_count);
    
    int failed = 0;
    for (int i = 0; i < config->dependency_count; i++) {
        printf("[%d/%d] %s\n", i + 1, config->dependency_count,
               config->dependencies[i].name);
        
        if (install_package_internal(config->dependencies[i].name,
                                    config->dependencies[i].version,
                                    false) != 0) {
            failed++;
        }
        printf("\n");
    }
    
    wyn_config_free(config);
    
    if (failed > 0) {
        printf("✗ %d packages failed to install\n", failed);
        return 1;
    }
    
    printf("✓ All packages installed successfully\n");
    return 0;
}

int package_list() {
    load_registry();
    
    char* pkg_dir = get_package_dir();
    if (!pkg_dir) {
        fprintf(stderr, "Error: Could not determine package directory\n");
        return 1;
    }
    
    printf("Installed packages (%s):\n\n", pkg_dir);
    
    if (global_registry.count == 0) {
        printf("  (No packages installed yet)\n");
        return 0;
    }
    
    for (int i = 0; i < global_registry.count; i++) {
        PackageInfo* pkg = &global_registry.packages[i];
        printf("  %s@%s\n", pkg->name, pkg->version);
        printf("    Path: %s\n", pkg->path);
    }
    
    printf("\nTotal: %d packages\n", global_registry.count);
    return 0;
}

int package_remove(const char* name) {
    load_registry();
    
    // Find package
    int found_idx = -1;
    for (int i = 0; i < global_registry.count; i++) {
        if (strcmp(global_registry.packages[i].name, name) == 0) {
            found_idx = i;
            break;
        }
    }
    
    if (found_idx < 0) {
        fprintf(stderr, "Package '%s' not found\n", name);
        return 1;
    }
    
    PackageInfo* pkg = &global_registry.packages[found_idx];
    printf("Removing %s@%s...\n", pkg->name, pkg->version);
    
    // Remove directory
    char cmd[2048];
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "rmdir /S /Q \"%s\" 2>nul", pkg->path);
#else
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\" 2>/dev/null", pkg->path);
#endif
    system(cmd);
    
    // Remove from registry
    for (int i = found_idx; i < global_registry.count - 1; i++) {
        global_registry.packages[i] = global_registry.packages[i + 1];
    }
    global_registry.count--;
    
    save_registry();
    
    printf("✓ Package removed\n");
    return 0;
}

int package_update(const char* project_dir) {
    printf("Updating packages...\n");
    
    // For now, just reinstall
    return package_install(project_dir);
}
