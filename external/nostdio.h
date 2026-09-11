#if !defined(_STDIO_H)
#define _STDIO_H

#include <stddef.h>
#include <stdarg.h>


#ifdef __cplusplus
extern "C" {
#endif

struct _IO_FILE;
/* The opaque type of streams.  This is the definition used elsewhere.  */
typedef struct _IO_FILE FILE;

int fprintf(FILE *,const char *__fmt, ...);
int printf(const char *__fmt, ...);
int vsnprintf(char *string, size_t length, const char *format, va_list args) __attribute__((format(printf, 3, 0)));
int snprintf(char *string, size_t length, const char *format, ...) __attribute__((format(printf, 3, 4)));
int sscanf(const char *buffer, const char *format, ...) __attribute__ ((format (scanf, 2, 3)));

#ifdef __cplusplus
}
#endif

#endif // _STDIO_H
