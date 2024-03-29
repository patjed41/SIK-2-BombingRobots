#ifndef ERR_H
#define ERR_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define ENSURE(x)                                                         \
    do {                                                                  \
        bool result = (x);                                                \
        if (!result) {                                                    \
            fprintf(stderr, "Error: %s was false in %s at %s:%d\n",       \
                #x, __func__, __FILE__, __LINE__);                        \
            exit(EXIT_FAILURE);                                           \
        }                                                                 \
    } while (0)

#define PRINT_ERRNO()                                                     \
    do {                                                                  \
        if (errno != 0) {                                                 \
            fprintf(stderr, "Error: errno %d in %s at %s:%d\n%s\n",       \
              errno, __func__, __FILE__, __LINE__, strerror(errno));      \
            exit(EXIT_FAILURE);                                           \
        }                                                                 \
    } while (0)

#define CHECK_ERRNO(x)                                                    \
    do {                                                                  \
        errno = 0;                                                        \
        (void) (x);                                                       \
        PRINT_ERRNO();                                                    \
    } while (0)

inline void fatal(const char *fmt, ...) {
    va_list fmt_args;

    fprintf(stderr, "Error: ");
    va_start(fmt_args, fmt);
    vfprintf(stderr, fmt, fmt_args);
    va_end(fmt_args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

#endif // ERR_H
