#ifndef REGISTRY_H
#define REGISTRY_H

int registry_search(const char *query);
int registry_info(const char *package);
int registry_versions(const char *package);
int registry_install(const char *package_spec);
int registry_publish(int dry_run);
int registry_resolve_version(const char *package, const char *constraint, char *resolved_version, size_t buf_size);

#endif
