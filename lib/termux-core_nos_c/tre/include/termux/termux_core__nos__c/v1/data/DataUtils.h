#ifndef LIBTERMUX_CORE__NOS__C__DATA_UTILS___H
#define LIBTERMUX_CORE__NOS__C__DATA_UTILS___H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif



/** Check if `str` starts with `prefix`. */
bool stringStartsWith(const char *string, const char *prefix);

/** Check if `str` ends with `suffix`. */
bool stringEndsWith(const char *string, const char *suffix);





int stringToInt(const char* string, int def, const char* logTag, const char *fmt, ...);





/**
 * Check if `string` matches the `pattern` regex.
 *
 * - https://man7.org/linux/man-pages/man0/regex.h.0p.html
 * - https://pubs.opengroup.org/onlinepubs/009696899/functions/regcomp.html
 *
 * @param string The string to match.
 * @param pattern The regex pattern to match with.
 * @param cflags The flags for `regcomp()` like `REG_EXTENDED`.
 * @return Returns `0` on match, `1` on no match and `-1` on other failures.
 */
int regexMatch(const char *string, const char *pattern, int cflags);



#ifdef __cplusplus
}
#endif

#endif // LIBTERMUX_CORE__NOS__C__DATA_UTILS___H
