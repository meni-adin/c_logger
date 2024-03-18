#include "logger.h"
#include <stdio.h>

#define BUF_SIZE 0x1000u

char g_buf[BUF_SIZE];

void
logger_init(void)
{
}

void
logger_log(LoggingLevel_t loggingLevel, const char *format, ...)
{
    printf("log something...\n");
}
