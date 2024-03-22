#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


typedef enum LoggingLevel_t
{
    LOG_LVL_TRACE,
    LOG_LVL_DEBUG,
    LOG_LVL_INFO,
    LOG_LVL_WARNING,
    LOG_LVL_ERROR,
} LoggingLevel_t;

#define LOG_TRACE(...) logger_log(LOG_LVL_TRACE, __VA_ARGS__)

void
logger_init(void);

void
logger_log(LoggingLevel_t, const char *format, ...);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LOGGER_H
