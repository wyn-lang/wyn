#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "semver.h"

int semver_parse(const char *str, SemVer *ver) {
    if (!str || !ver) return -1;
    
    // Skip leading 'v' if present
    if (*str == 'v') str++;
    
    // Parse major.minor.patch
    char *end;
    ver->major = strtol(str, &end, 10);
    if (*end != '.') return -1;
    
    str = end + 1;
    ver->minor = strtol(str, &end, 10);
    if (*end != '.') return -1;
    
    str = end + 1;
    ver->patch = strtol(str, &end, 10);
    
    return 0;
}

int constraint_parse(const char *str, VersionConstraint *constraint) {
    if (!str || !constraint) return -1;
    
    // Check for wildcard
    if (strcmp(str, "*") == 0) {
        constraint->type = CONSTRAINT_ANY;
        return 0;
    }
    
    // Check for caret (^)
    if (*str == '^') {
        constraint->type = CONSTRAINT_CARET;
        return semver_parse(str + 1, &constraint->version);
    }
    
    // Check for tilde (~)
    if (*str == '~') {
        constraint->type = CONSTRAINT_TILDE;
        return semver_parse(str + 1, &constraint->version);
    }
    
    // Check for >= 
    if (str[0] == '>' && str[1] == '=') {
        constraint->type = CONSTRAINT_GTE;
        return semver_parse(str + 2, &constraint->version);
    }
    
    // Default: exact version
    constraint->type = CONSTRAINT_EXACT;
    return semver_parse(str, &constraint->version);
}

int semver_compare(const SemVer *a, const SemVer *b) {
    if (a->major != b->major) return a->major - b->major;
    if (a->minor != b->minor) return a->minor - b->minor;
    return a->patch - b->patch;
}

int semver_satisfies(const SemVer *ver, const VersionConstraint *constraint) {
    switch (constraint->type) {
        case CONSTRAINT_ANY:
            return 1;
            
        case CONSTRAINT_EXACT:
            return semver_compare(ver, &constraint->version) == 0;
            
        case CONSTRAINT_CARET: {
            // ^1.2.3 means >=1.2.3 and <2.0.0
            if (ver->major != constraint->version.major) return 0;
            return semver_compare(ver, &constraint->version) >= 0;
        }
        
        case CONSTRAINT_TILDE: {
            // ~1.2.3 means >=1.2.3 and <1.3.0
            if (ver->major != constraint->version.major) return 0;
            if (ver->minor != constraint->version.minor) return 0;
            return ver->patch >= constraint->version.patch;
        }
        
        case CONSTRAINT_GTE:
            return semver_compare(ver, &constraint->version) >= 0;
    }
    
    return 0;
}
