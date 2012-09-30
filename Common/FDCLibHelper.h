#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int strcasecmp(const char *str1, const char *str2);

int strncasecmp(const char *str1, const char *str2, int n);

const char *strcasestr(const char *s, const char *pattern);


#ifdef __cplusplus
}
#endif