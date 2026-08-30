#pragma once

#define LOG_MSG_SIZE (4096)

unsigned int log_minimum_level = LOG_DEBUG;

const char *log_print_level_s(unsigned int level)
{
    switch (level) {
    case LOG_ERROR:
        return "error";
    case LOG_WARN:
        return "warn";
    case LOG_INFO:
        return "info";
    case LOG_DEBUG:
        return "debug";
    case LOG_TRACE:
        return "trace";
    default:
        return "<unknown>";
    }
}

void log_vmsg(unsigned int level, const char *format, va_list ap);
void log_msg(unsigned int level, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    log_vmsg(level, format, ap);
    va_end(ap);
}

void log_vmsg(unsigned int level, const char *format, va_list ap)
{
    int ret;
    char buffer[LOG_MSG_SIZE];

    if (log_minimum_level < level) {
        return;
    }

    ret = stbsp_vsnprintf(buffer, sizeof(buffer), format, ap);
    if (ret <= 0) {
        log_error("log: Invalid format '%s'", format);
        return;
    }

    const char *trimmed = "";
    if (sizeof(buffer) < ret) {
        trimmed = "(trimmed...)";
    }

    UtcTime time;
    if (sys_get_utc_time(&time) != 0) {
        memset(&time, 0, sizeof(time));
    }

    ret = fprintf(
        stderr,
        "[%04d-%02d-%02dT%02d:%02d:%02d.%03d] %5s: %s%s\n",
        time.year, time.month, time.day,
        time.hour, time.minute, time.second,
        time.millisecond,
        log_print_level_s(level),
        buffer,
        trimmed
    );

    if (ret < 0) {
        log_error("log: Failed to print a log");
    }

    fflush(stderr);
}

void log_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_vinfo(fmt, ap);
    va_end(ap);
}

void log_debug(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_vdebug(fmt, ap);
    va_end(ap);
}

void log_trace(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_vtrace(fmt, ap);
    va_end(ap);
}

void log_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_verror(fmt, ap);
    va_end(ap);
}

void log_warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_vwarn(fmt, ap);
    va_end(ap);
}

void log_vinfo(const char *format, va_list ap)
{
    log_vmsg(LOG_INFO, format, ap);
}

void log_vdebug(const char *format, va_list ap)
{
    log_vmsg(LOG_DEBUG, format, ap);
}

void log_vtrace(const char *format, va_list ap)
{
    log_vmsg(LOG_TRACE, format, ap);
}

void log_verror(const char *format, va_list ap)
{
    log_vmsg(LOG_ERROR, format, ap);
}

void log_vwarn(const char *format, va_list ap)
{
    log_vmsg(LOG_WARN, format, ap);
}
