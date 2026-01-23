// Simple TOML parser for wyn.toml
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "toml.h"

static char* trim(char* str) {
    while (isspace(*str)) str++;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    *(end + 1) = '\0';
    return str;
}

static char* parse_value(char* value) {
    value = trim(value);
    if (*value == '"') {
        value++;
        char* end = strchr(value, '"');
        if (end) *end = '\0';
    }
    return strdup(value);
}

WynConfig* wyn_config_parse(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) return NULL;

    WynConfig* config = calloc(1, sizeof(WynConfig));
    char line[1024];
    char current_section[64] = "";

    while (fgets(line, sizeof(line), f)) {
        char* trimmed = trim(line);
        
        // Skip empty lines and comments
        if (*trimmed == '\0' || *trimmed == '#') continue;
        
        // Section header
        if (*trimmed == '[') {
            char* end = strchr(trimmed, ']');
            if (end) {
                *end = '\0';
                strncpy(current_section, trimmed + 1, sizeof(current_section) - 1);
            }
            continue;
        }
        
        // Key = value
        char* eq = strchr(trimmed, '=');
        if (!eq) continue;
        
        *eq = '\0';
        char* key = trim(trimmed);
        char* value = parse_value(eq + 1);
        
        if (strcmp(current_section, "project") == 0) {
            if (strcmp(key, "name") == 0) config->project.name = value;
            else if (strcmp(key, "version") == 0) config->project.version = value;
            else if (strcmp(key, "entry") == 0) config->project.entry = value;
            else if (strcmp(key, "author") == 0) config->project.author = value;
            else if (strcmp(key, "description") == 0) config->project.description = value;
            else free(value);
        } else {
            free(value);
        }
    }
    
    fclose(f);
    return config;
}

void wyn_config_free(WynConfig* config) {
    if (!config) return;
    free(config->project.name);
    free(config->project.version);
    free(config->project.entry);
    free(config->project.author);
    free(config->project.description);
    for (int i = 0; i < config->dependency_count; i++) {
        free(config->dependencies[i].name);
        free(config->dependencies[i].version);
    }
    free(config->dependencies);
    free(config);
}

const char* wyn_config_get_name(WynConfig* config) {
    return config ? config->project.name : NULL;
}

const char* wyn_config_get_version(WynConfig* config) {
    return config ? config->project.version : NULL;
}
