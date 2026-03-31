#ifndef PLATFORM_H_
#define PLATFORM_H_

#include <stddef.h>
#include <stdbool.h>

void namo_get_user_data_dir(char *out, size_t cap);
void namo_get_user_config_dir(char *out, size_t cap);
void namo_path_join(char *out, size_t cap, const char *a, const char *b);
bool namo_file_exists(const char *path);
void namo_normalize_path(char *path);
const char *namo_getenv(const char *name);

#endif /* PLATFORM_H_ */
