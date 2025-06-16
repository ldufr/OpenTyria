#pragma once

int sys_init(void);
int sys_free(void);
int sys_errno(void);

typedef struct UtcTime {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int millisecond;
} UtcTime;

int sys_get_utc_time(UtcTime *time);
uint64_t sys_get_monotonic_time_ms();

int  sys_socket(uintptr_t *result, int af, int type, int protocol);
void sys_closesocket(uintptr_t fd);

int  sys_enable_nonblocking(uintptr_t fd, bool enable);
int  sys_set_reuseaddr(uintptr_t fd, bool enable);
int  sys_bind(uintptr_t fd, const struct sockaddr *addr, int namelen);
int  sys_listen(uintptr_t fd, int backlog);
int  sys_accept(uintptr_t *result, uintptr_t fd, struct sockaddr *addr, int *addrlen);
int  sys_recv(uintptr_t fd, uint8_t *buffer, size_t size, size_t *result);
int  sys_send(uintptr_t fd, const uint8_t *buffer, size_t size, size_t *result);
int  sys_getsockname(uintptr_t fd, struct sockaddr_storage *result);
int  sys_getpeername(uintptr_t fd, struct sockaddr_storage *result);

bool sys_would_block(int err);

int  sys_getrandom(uint8_t *buffer, size_t size);

typedef void (*sys_thread_start_t)(void *);
typedef struct Thread {
    sys_thread_start_t start;
    void *param;
    uintptr_t handle;
} Thread;

int sys_thread_create(Thread *thread, sys_thread_start_t start, void *param);
int sys_thread_join(Thread *thread);

typedef struct Mutex {
#if PLATFORM_WINDOWS
    CRITICAL_SECTION section;
#elif PLATFORM_LINUX
    pthread_mutex_t section;
#else
    #error "Unsupported platform"
#endif
} Mutex;

int  sys_mutex_init(Mutex *mtx);
int  sys_mutex_free(Mutex *mtx);
void sys_mutex_lock(Mutex *mtx);
bool sys_mutex_try_lock(Mutex *mtx);
void sys_mutex_unlock(Mutex *mtx);
