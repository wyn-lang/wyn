// Semantic versioning parser and resolver
#ifndef SEMVER_H
#define SEMVER_H

typedef struct {
    int major;
    int minor;
    int patch;
} SemVer;

typedef enum {
    CONSTRAINT_EXACT,      // 1.0.0
    CONSTRAINT_CARET,      // ^1.0.0 (>=1.0.0, <2.0.0)
    CONSTRAINT_TILDE,      // ~1.0.0 (>=1.0.0, <1.1.0)
    CONSTRAINT_GTE,        // >=1.0.0
    CONSTRAINT_ANY         // *
} ConstraintType;

typedef struct {
    ConstraintType type;
    SemVer version;
} VersionConstraint;

// Parse version string (e.g., "1.2.3")
int semver_parse(const char *str, SemVer *ver);

// Parse constraint string (e.g., "^1.0.0", "~1.2.3", ">=2.0.0")
int constraint_parse(const char *str, VersionConstraint *constraint);

// Check if version satisfies constraint
int semver_satisfies(const SemVer *ver, const VersionConstraint *constraint);

// Compare versions (-1: a < b, 0: a == b, 1: a > b)
int semver_compare(const SemVer *a, const SemVer *b);

#endif // SEMVER_H
