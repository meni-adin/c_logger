#include "logger.h"
#include <stdio.h>
#include <stdarg.h>

typedef struct Logger
{
    uint32_t enabled : 1;
    const char *fileName;
    // FILE
} Logger;

void
logger_init(void)
{
}

void
logger_log(LoggingLevel_t loggingLevel, const char *format, ...)
{
    va_list list;
    va_start(list, format);
    vfprintf(stdout, format, list);
    va_end(list);
}
