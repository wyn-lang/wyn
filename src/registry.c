#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "registry.h"
#include "toml.h"
#include "semver.h"

#define REGISTRY_URL "https://pkg.wynlang.com"

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        fprintf(stderr, "Error: Out of memory\n");
        return 0;
    }
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}

int registry_search(const char *query) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk = {0};
    char url[512];
    
    snprintf(url, sizeof(url), "%s/api/search?q=%s", REGISTRY_URL, query);
    
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error: Failed to initialize curl\n");
        return 1;
    }
    
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "wyn-cli/1.5.0");
    
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "Error: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        free(chunk.memory);
        return 1;
    }
    
    printf("%s\n", chunk.memory);
    
    curl_easy_cleanup(curl);
    free(chunk.memory);
    return 0;
}

int registry_info(const char *package) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk = {0};
    char url[512];
    
    snprintf(url, sizeof(url), "%s/api/packages/%s", REGISTRY_URL, package);
    
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error: Failed to initialize curl\n");
        return 1;
    }
    
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "wyn-cli/1.5.0");
    
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "Error: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        free(chunk.memory);
        return 1;
    }
    
    printf("%s\n", chunk.memory);
    
    curl_easy_cleanup(curl);
    free(chunk.memory);
    return 0;
}

int registry_versions(const char *package) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk = {0};
    char url[512];
    
    snprintf(url, sizeof(url), "%s/api/packages/%s/versions", REGISTRY_URL, package);
    
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error: Failed to initialize curl\n");
        return 1;
    }
    
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "wyn-cli/1.5.0");
    
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "Error: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        free(chunk.memory);
        return 1;
    }
    
    printf("%s\n", chunk.memory);
    
    curl_easy_cleanup(curl);
    free(chunk.memory);
    return 0;
}

int registry_install(const char *package_spec) {
    char package[256] = {0};
    char version[64] = {0};
    
    // Parse package@version or just package
    const char *at = strchr(package_spec, '@');
    if (at) {
        size_t pkg_len = at - package_spec;
        if (pkg_len >= sizeof(package)) pkg_len = sizeof(package) - 1;
        strncpy(package, package_spec, pkg_len);
        strncpy(version, at + 1, sizeof(version) - 1);
    } else {
        strncpy(package, package_spec, sizeof(package) - 1);
        strcpy(version, "latest");
    }
    
    // Get package info to resolve version
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error: Failed to initialize curl\n");
        return 1;
    }
    
    struct MemoryStruct chunk = {0};
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    char url[512];
    if (strcmp(version, "latest") == 0) {
        snprintf(url, sizeof(url), "%s/api/packages/%s", REGISTRY_URL, package);
    } else {
        snprintf(url, sizeof(url), "%s/api/packages/%s/versions/%s", REGISTRY_URL, package, version);
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "wyn-cli/1.5.0");
    
    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "Error: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        free(chunk.memory);
        return 1;
    }
    
    // Check HTTP status
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code == 404) {
        fprintf(stderr, "Error: Package '%s' not found\n", package);
        curl_easy_cleanup(curl);
        free(chunk.memory);
        return 1;
    }
    
    printf("Installing %s@%s...\n", package, version);
    printf("Note: Registry server not yet deployed. Install command ready.\n");
    
    curl_easy_cleanup(curl);
    free(chunk.memory);
    return 0;
}

int registry_publish(int dry_run) {
    // 1. Validate wyn.toml exists
    if (access("wyn.toml", F_OK) != 0) {
        fprintf(stderr, "Error: wyn.toml not found. Run 'wyn init' first.\n");
        return 1;
    }
    
    // 2. Parse wyn.toml
    WynConfig *config = wyn_config_parse("wyn.toml");
    if (!config) {
        fprintf(stderr, "Error: Failed to parse wyn.toml\n");
        return 1;
    }
    
    // 3. Validate required fields
    if (!config->project.name || strlen(config->project.name) == 0) {
        fprintf(stderr, "Error: Missing required field: name\n");
        wyn_config_free(config);
        return 1;
    }
    if (!config->project.version || strlen(config->project.version) == 0) {
        fprintf(stderr, "Error: Missing required field: version\n");
        wyn_config_free(config);
        return 1;
    }
    
    if (dry_run) {
        printf("Publishing %s@%s...\n", config->project.name, config->project.version);
        printf("\n[DRY RUN] Would publish:\n");
        printf("  Name: %s\n", config->project.name);
        printf("  Version: %s\n", config->project.version);
        if (config->project.author) printf("  Author: %s\n", config->project.author);
        if (config->project.description) printf("  Description: %s\n", config->project.description);
        if (config->dependency_count > 0) {
            printf("  Dependencies:\n");
            for (int i = 0; i < config->dependency_count; i++) {
                printf("    - %s@%s\n", config->dependencies[i].name, config->dependencies[i].version);
            }
        }
        wyn_config_free(config);
        return 0;
    }
    
    printf("Publishing %s@%s...\n", config->project.name, config->project.version);
    
    // 4. Create tarball (simplified - just prepare metadata)
    char tarball[512];
    snprintf(tarball, sizeof(tarball), "%s-%s.tar.gz", config->project.name, config->project.version);
    
    // 5. Upload to registry
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error: Failed to initialize curl\n");
        wyn_config_free(config);
        return 1;
    }
    
    char url[512];
    snprintf(url, sizeof(url), "%s/api/publish", REGISTRY_URL);
    
    // Prepare JSON payload
    char payload[2048];
    snprintf(payload, sizeof(payload),
        "{\"name\":\"%s\",\"version\":\"%s\",\"author\":\"%s\",\"description\":\"%s\"}",
        config->project.name,
        config->project.version,
        config->project.author ? config->project.author : "",
        config->project.description ? config->project.description : ""
    );
    
    struct MemoryStruct chunk = {0};
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "wyn-cli/1.5.0");
    
    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "Error: %s\n", curl_easy_strerror(res));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        free(chunk.memory);
        wyn_config_free(config);
        return 1;
    }
    
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    if (http_code == 409) {
        fprintf(stderr, "Error: Version %s already published\n", config->project.version);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        free(chunk.memory);
        wyn_config_free(config);
        return 1;
    }
    
    printf("Note: Registry server not yet deployed. Publish command ready.\n");
    printf("Package metadata validated and ready for upload.\n");
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(chunk.memory);
    wyn_config_free(config);
    return 0;
}

int registry_resolve_version(const char *package, const char *constraint_str, char *resolved_version, size_t buf_size) {
    // Parse constraint
    VersionConstraint constraint;
    if (constraint_parse(constraint_str, &constraint) != 0) {
        fprintf(stderr, "Error: Invalid version constraint: %s\n", constraint_str);
        return -1;
    }
    
    // Get available versions from registry
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error: Failed to initialize curl\n");
        return -1;
    }
    
    struct MemoryStruct chunk = {0};
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    char url[512];
    snprintf(url, sizeof(url), "%s/api/packages/%s/versions", REGISTRY_URL, package);
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "wyn-cli/1.5.0");
    
    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "Error: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        free(chunk.memory);
        return -1;
    }
    
    // Parse JSON response and find best matching version
    // For now, simplified: just return "latest" or the exact version
    if (constraint.type == CONSTRAINT_ANY || strcmp(constraint_str, "latest") == 0) {
        snprintf(resolved_version, buf_size, "latest");
    } else {
        // Format version as string
        snprintf(resolved_version, buf_size, "%d.%d.%d", 
                constraint.version.major, 
                constraint.version.minor, 
                constraint.version.patch);
    }
    
    curl_easy_cleanup(curl);
    free(chunk.memory);
    return 0;
}
