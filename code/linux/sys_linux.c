#pragma once

FILE *g_Random;

int sys_init(void)
{
    if ((g_Random = fopen("/dev/urandom", "rb")) == NULL) {
        return 1;
    }

    return 0;
}

int sys_free(void)
{
    if (g_Random)
        free(g_Random);
    return 0;
}

int sys_errno(void)
{
    return errno;
}

int sys_get_utc_time(UtcTime *time)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        log_error("clock_gettime failed, err: %d", errno);
        return ERR_UNSUCCESSFUL;
    }

    struct tm system_time;
    if (gmtime_r(&ts.tv_sec, &system_time) == NULL) {
        log_error("gmtime_r failed, err: %d", errno);
        return ERR_UNSUCCESSFUL;
    }

    time->year = system_time.tm_year + 1900;
    time->month = system_time.tm_mon + 1;
    time->day = system_time.tm_mday;
    time->hour = system_time.tm_hour;
    time->minute = system_time.tm_min;
    time->second = system_time.tm_sec;
    time->millisecond = ts.tv_nsec / 1000;
    return 0;
}

uint64_t sys_get_monotonic_time_ms()
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        log_error("clock_gettime(CLOCK_MONOTONIC) failed, err: %d", errno);
        abort();
    }
    return (uint64_t)ts.tv_sec + (uint64_t)(ts.tv_nsec / 1000);
}

int sys_socket(uintptr_t *result, int af, int type, int protocol)
{
    int fd;
    if ((fd = socket(af, type, protocol)) != -1) {
        *result = (uintptr_t) fd;
        return 0;
    } else {
        return errno;
    }
}

void sys_closesocket(uintptr_t fd)
{
    close((int)fd);
}

int sys_enable_nonblocking(uintptr_t fd, bool enable)
{
    int err;

    if ((err = fcntl((int)fd, F_GETFL)) == -1) {
        log_error("fcntl(F_GETFL) failed, err: %d", errno);
        return ERR_UNSUCCESSFUL;
    }

    int mask = enable ? (err | O_NONBLOCK) : (err & ~O_NONBLOCK);
    if (fcntl((int)fd, F_SETFL, O_NONBLOCK, mask) == -1) {
        log_error("fcntl(F_SETFL) failed, err: %d", errno);
        return ERR_UNSUCCESSFUL;
    }

    return 0;
}

int sys_set_reuseaddr(uintptr_t fd, bool enable)
{
    int optval = enable ? 1 : 0;
    if (setsockopt((int)fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0) {
        log_error("setsockopt(SOL_SOCKET, SO_REUSEADDR) failed, err: %d", sys_errno);
        return ERR_UNSUCCESSFUL;
    }
    return 0;
}

int sys_bind(uintptr_t fd, const struct sockaddr *addr, int namelen)
{
    if (bind((int)fd, addr, namelen) != -1) {
        return 0;
    } else {
        return errno;
    }
}

int sys_listen(uintptr_t fd, int backlog)
{
    if (listen((int)fd, backlog) != -1) {
        return 0;
    } else {
        return errno;
    }
}

int sys_accept(uintptr_t *result, uintptr_t fd, struct sockaddr *addr, int *addrlen)
{
    socklen_t socklen = (socklen_t)*addrlen;

    int socket;
    if ((socket = accept((int)fd, addr, &socklen)) != -1) {
        *addrlen = (int)socklen;
        *result = socket;
        return 0;
    } else {
        return errno;
    }
}

int sys_recv(uintptr_t fd, uint8_t *buffer, size_t size, size_t *result)
{
    int ret;
    int isize = (int)min_size_t(size, INT_MAX);
    if ((ret = recv((int)fd, (char *) buffer, isize, 0)) < 0) {
        return errno;
    }

    *result = (size_t) ret;
    return 0;
}

int sys_send(uintptr_t fd, const uint8_t *buffer, size_t size, size_t *result)
{
    int ret;
    int isize = (int)min_size_t(size, INT_MAX);
    if ((ret = send((int)fd, (const char *) buffer, isize, 0)) < 0) {
        return errno;
    }

    *result = (size_t) ret;
    return 0;
}

int sys_getsockname(uintptr_t fd, struct sockaddr_storage *result)
{
    int ret;
    socklen_t len = sizeof(*result);
    if ((ret = getsockname((int)fd, (struct sockaddr *)result, &len)) != 0) {
        return ERR_UNSUCCESSFUL;
    }
    return ERR_OK;
}

int sys_getpeername(uintptr_t fd, struct sockaddr_storage *result)
{
    int ret;
    socklen_t len = sizeof(*result);
    if ((ret = getpeername((int)fd, (struct sockaddr *)result, &len)) != 0) {
        return ERR_UNSUCCESSFUL;
    }
    return ERR_OK;
}

bool sys_would_block(int err)
{
    return err == EWOULDBLOCK;
}

int sys_getrandom(uint8_t *buffer, size_t size)
{
    if (fread(buffer, size, 1, g_Random) != 1)
        return 1;
    return 0;
}

void* ThreadProc(void* param)
{
    Thread *thread = (Thread *) param;
    thread->start(thread->param);
    return 0;
}

int sys_thread_create(Thread *thread, sys_thread_start_t start, void *param)
{
    int err;

    thread->start = start;
    thread->param = param;

    pthread_attr_t attr;
    if ((err = pthread_attr_init(&attr)) != 0) {
        log_error("pthread_attr_init failed, err: %d", err);
        return ERR_UNSUCCESSFUL;
    }

    pthread_t hThread;
    err = pthread_create(&hThread, &attr, ThreadProc, thread);
    pthread_attr_destroy(&attr);

    if (err != 0) {
        log_error("pthread_create failed, err: %d", err);
        return ERR_UNSUCCESSFUL;
    }

    thread->handle = (uintptr_t) hThread;
    return 0;
}

int sys_thread_join(Thread *thread)
{
    if (thread->handle == 0) {
        return ERR_UNSUCCESSFUL;
    }

    pthread_t hThread = (pthread_t) thread->handle;
    void* retval;
    if (pthread_join(hThread, &retval) != 0) {
        return ERR_UNSUCCESSFUL;
    }

    thread->handle = 0;
    return 0;
}

int sys_mutex_init(Mutex *mtx)
{
    int err;

    pthread_mutexattr_t attr;
    if ((err = pthread_mutexattr_init(&attr)) != 0) {
        log_error("pthread_mutexattr_init failed, err: %d", err);
        return ERR_UNSUCCESSFUL;
    }

    if ((err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE)) != 0) {
        pthread_mutexattr_destroy(&attr);
        log_error("failed to make the mutex recursive, err: %d", err);
        return ERR_UNSUCCESSFUL;
    }

    err = pthread_mutex_init(&mtx->section, &attr);
    pthread_mutexattr_destroy(&attr);

    if (err != 0) {
        log_error("pthread_mutex_init failed, err: %d", err);
    }

    return 0;
}

int sys_mutex_free(Mutex *mtx)
{
    int err;
    if ((err = pthread_mutex_destroy(&mtx->section)) != 0) {
        log_error("pthread_mutex_destroy failed, err: %d", err);
        return ERR_UNSUCCESSFUL;
    }
    return 0;
}

void sys_mutex_lock(Mutex *mtx)
{
    int err;
    if ((err = pthread_mutex_lock(&mtx->section)) != 0) {
        log_error("pthread_mutex_lock failed, err: %d", err);
        abort();
    }
}

bool sys_mutex_try_lock(Mutex *mtx)
{
    int err;
    if ((err = pthread_mutex_trylock(&mtx->section)) != 0) {
        if (err != EBUSY) {
            log_error("pthread_mutex_lock failed, err: %d", err);
            abort();
        }
        return false;
    }
    return true;
}

void sys_mutex_unlock(Mutex *mtx)
{
    int err;
    if ((err = pthread_mutex_lock(&mtx->section)) != 0) {
        log_error("pthread_mutex_lock failed, err: %d", err);
        abort();
    }
}
