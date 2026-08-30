#pragma once

enum
{
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,
    LOG_TRACE,
    
    LOG_LEVEL_COUNT,
};

void log_msg(unsigned int level, const char *format, ...);
void log_vmsg(unsigned int level, const char *format, va_list ap);

void log_info(const char *fmt, ...);
void log_debug(const char *fmt, ...);
void log_error(const char *fmt, ...);
void log_trace(const char *fmt, ...);
void log_warn(const char *fmt, ...);

void log_vinfo(const char *format, va_list ap);
void log_vdebug(const char *format, va_list ap);
void log_verror(const char *format, va_list ap);
void log_vtrace(const char *format, va_list ap);
void log_vwarn(const char *format, va_list ap);
