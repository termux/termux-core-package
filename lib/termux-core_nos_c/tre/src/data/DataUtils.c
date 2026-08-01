#define _GNU_SOURCE
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <termux/termux_core__nos__c/v1/TermuxCoreLibraryConfig.h>
#include <termux/termux_core__nos__c/v1/data/DataUtils.h>
#include <termux/termux_core__nos__c/v1/logger/Logger.h>

static const char* LOG_TAG = "DataUtils";



bool stringEquals(const char* string1, const char* string2) {
    return stringEqualsE(string1, string2, /* `ensureNotEmpty` */ true);
}

bool stringEqualsE(const char* string1, const char* string2, bool ensureNotEmpty) {
    if (string1 == NULL || string2 == NULL) {
        return false;
    }

    if (ensureNotEmpty) {
        if (*string1 == '\0' || *string2 == '\0') {
            return false;
        }
    } else {
        if (*string1 == '\0' && *string2 == '\0') {
            return true;
        } else if ((*string1 == '\0' && *string2 != '\0') || (*string1 != '\0' && *string2 == '\0')) {
            return false;
        }
    }

    return strcmp(string1, string2) == 0;
}


bool stringStartsWith(const char *string, const char *prefix) {
    if (string == NULL || prefix == NULL) {
        return false;
    }

    return *string != '\0' && *prefix != '\0' && (strncmp(string, prefix, strlen(prefix)) == 0);
}

bool stringEndsWith(const char *string, const char *suffix) {
    if (string == NULL || suffix == NULL) {
        return false;
    }

    int strLength = strlen(string);
    int suffixLength = strlen(suffix);

    return *string != '\0' && *suffix != '\0' && (strLength >= suffixLength) && \
        (strcmp(string + (strLength - suffixLength), suffix) == 0);
}





int stringToInt(const char* string, int def, const char* logTag, const char *fmt, ...) {
    // We do not use atoi as it will not set errno or return errors.
    // - https://man7.org/linux/man-pages/man3/atoi.3.html
    // - https://man7.org/linux/man-pages/man3/strtol.3.html
    char *end;
    errno = 0;
    long value = strtol(string, &end, 10);
    if (end == string || *end != '\0' || errno != 0 || value < 0 || value > INT_MAX) {
        if (libtermux_core__nos__c__getIsRunningTests()) {
            if (fmt != NULL) {
                va_list args;
                va_start(args, fmt);
                logStrerrorMessage(errno, logTag, fmt, args);
                va_end(args);
            }
        }
        errno = 0;
        return def;
    }

    return (int) value;
}





int regexMatch(const char *string, const char *pattern, int cflags) {
    if (string == NULL) {
        return 1;
    }

    regex_t regex;
    int result;
    char msgbuf[100];

    // Compile regular expression.
    result = regcomp(&regex, pattern, cflags);
    if (result != 0) {
        regerror(result, &regex, msgbuf, sizeof(msgbuf));
        logErrorDebug(LOG_TAG, "Failed to compile regex '%s': %s", pattern, msgbuf);
        return -1;
    }

    // Match regular expression.
    result = regexec(&regex, string, 0, NULL, 0);
    int match;
    if (result == 0) {
        match = 0;
    } else if (result == REG_NOMATCH) {
        match = 1;
    } else {
        regerror(result, &regex, msgbuf, sizeof(msgbuf));
        logErrorDebug(LOG_TAG, "Regex match failed '%s': %s", pattern, msgbuf);
        match =-1;
    }

    regfree(&regex);

    return match;
}
