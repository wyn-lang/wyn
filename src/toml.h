// Simple TOML parser for wyn.toml
// Supports basic key=value and [sections]

#ifndef TOML_H
#define TOML_H

typedef struct {
    char* name;
    char* version;
    char* entry;
    char* author;
    char* description;
} WynProject;

typedef struct {
    char* name;
    char* version;
} WynDependency;

typedef struct {
    WynProject project;
    WynDependency* dependencies;
    int dependency_count;
} WynConfig;

// Parse wyn.toml file
WynConfig* wyn_config_parse(const char* filename);

// Free config
void wyn_config_free(WynConfig* config);

// Get project name
const char* wyn_config_get_name(WynConfig* config);

// Get project version
const char* wyn_config_get_version(WynConfig* config);

#endif // TOML_H
