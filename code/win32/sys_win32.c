#pragma once

int sys_init(void)
{
    int ret;

    WSADATA data;
    if ((ret = WSAStartup(MAKEWORD(2, 2), &data)) != 0) {
        return ret;
    }

    return 0;
}

int sys_free(void)
{
    int ret;
    if ((ret = WSACleanup()) != 0) {
        log_warn("WSACleanup failed, err: %d", WSAGetLastError());
    }

    return 0;
}

int sys_errno(void)
{
    return (int)GetLastError();
}

int sys_get_utc_time(UtcTime *time)
{
    SYSTEMTIME system_time;
    GetSystemTime(&system_time);
    time->year = system_time.wYear;
    time->month = system_time.wMonth;
    time->day = system_time.wDay;
    time->hour = system_time.wHour;
    time->minute = system_time.wMinute;
    time->second = system_time.wSecond;
    time->millisecond = system_time.wMilliseconds;
    return 0;
}

uint64_t sys_get_monotonic_time_ms()
{
    return GetTickCount64();
}

int sys_socket(uintptr_t *result, int af, int type, int protocol)
{
    SOCKET fd;
    if ((fd = socket(af, type, protocol)) != INVALID_SOCKET) {
        *result = (uintptr_t) fd;
        return 0;
    } else {
        return WSAGetLastError();
    }
}

void sys_closesocket(uintptr_t fd)
{
    closesocket(fd);
}

int sys_enable_nonblocking(uintptr_t fd, bool enable)
{
    u_long flag = enable ? 1 : 0;
    if (ioctlsocket(fd, FIONBIO, &flag) != SOCKET_ERROR) {
        return 0;
    } else {
        return WSAGetLastError();
    }
}

int sys_connect(uintptr_t fd, const struct sockaddr *addr, int namelen)
{
    if (connect(fd, addr, namelen) != SOCKET_ERROR) {
        return 0;
    } else {
        return WSAGetLastError();
    }
}

int sys_bind(uintptr_t fd, const struct sockaddr *addr, int namelen)
{
    if (bind(fd, addr, namelen) != SOCKET_ERROR) {
        return 0;
    } else {
        return WSAGetLastError();
    }
}

int sys_listen(uintptr_t fd, int backlog)
{
    if (listen(fd, backlog) != SOCKET_ERROR) {
        return 0;
    } else {
        return WSAGetLastError();
    }
}

int sys_accept(uintptr_t *result, uintptr_t fd, struct sockaddr *addr, int *addrlen)
{
    SOCKET socket;
    if ((socket = accept(fd, addr, addrlen)) != INVALID_SOCKET) {
        *result = socket;
        return 0;
    } else {
        return WSAGetLastError();
    }
}

int sys_recv(uintptr_t fd, uint8_t *buffer, size_t size, size_t *result)
{
    int ret;
    int isize = (int)min_size_t(size, INT_MAX);
    if ((ret = recv(fd, (char *) buffer, isize, 0)) == SOCKET_ERROR) {
        return WSAGetLastError();
    }

    *result = (size_t) ret;
    return 0;
}

int sys_send(uintptr_t fd, const uint8_t *buffer, size_t size, size_t *result)
{
    int ret;
    int isize = (int)min_size_t(size, INT_MAX);
    if ((ret = send(fd, (const char *) buffer, isize, 0)) == SOCKET_ERROR) {
        return WSAGetLastError();
    }

    *result = (size_t) ret;
    return 0;
}

int sys_getsockname(uintptr_t fd, struct sockaddr_storage *result)
{
    int ret;
    socklen_t len = sizeof(*result);
    if ((ret = getsockname(fd, (struct sockaddr *)result, &len)) != 0) {
        return ERR_UNSUCCESSFUL;
    }
    return ERR_OK;
}

int sys_getpeername(uintptr_t fd, struct sockaddr_storage *result)
{
    int ret;
    socklen_t len = sizeof(*result);
    if ((ret = getpeername(fd, (struct sockaddr *)result, &len)) != 0) {
        return ERR_UNSUCCESSFUL;
    }
    return ERR_OK;
}

bool sys_would_block(int err)
{
    return err == WSAEWOULDBLOCK;
}

int sys_getrandom(uint8_t *buffer, size_t size)
{
    while (size != 0) {
        ULONG cbBuffer = (ULONG)min_size_t(ULONG_MAX, size);
        NTSTATUS status = BCryptGenRandom(
            NULL,
            buffer,
            cbBuffer,
            BCRYPT_USE_SYSTEM_PREFERRED_RNG);

        if (!BCRYPT_SUCCESS(status)) {
            return 1;
        }

        buffer += (size_t)cbBuffer;
        size -= (size_t)cbBuffer;
    }

    return 0;
}

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
    Thread *thread = (Thread *) lpParameter;
    thread->start(thread->param);
    return 0;
}

int sys_thread_create(Thread *thread, sys_thread_start_t start, void *param)
{
    thread->start = start;
    thread->param = param;

    HANDLE hThread = CreateThread(NULL, 0, ThreadProc, thread, 0, NULL);
    if (hThread == NULL) {
        return (int) GetLastError();
    }

    thread->handle = (uintptr_t) hThread;
    return 0;
}

int sys_thread_join(Thread *thread)
{
    if (thread->handle == 0) {
        return ERROR_INVALID_PARAMETER;
    }

    HANDLE hThread = (HANDLE) thread->handle;
    DWORD dwReason = WaitForSingleObject(hThread, 0);

    if (dwReason == WAIT_FAILED) {
        return (int) GetLastError();
    }

    if (dwReason != WAIT_OBJECT_0) {
        log_error("WaitForSingleObject failed");
    }

    CloseHandle((HANDLE) thread->handle);
    thread->handle = 0;

    return 0;
}

int sys_mutex_init(Mutex *mtx)
{
    InitializeCriticalSection(&mtx->section);
    return 0;
}

int sys_mutex_free(Mutex *mtx)
{
    DeleteCriticalSection(&mtx->section);
    return 0;
}

void sys_mutex_lock(Mutex *mtx)
{
    EnterCriticalSection(&mtx->section);
}

bool sys_mutex_try_lock(Mutex *mtx)
{
    return TryEnterCriticalSection(&mtx->section) == TRUE;
}

void sys_mutex_unlock(Mutex *mtx)
{
    LeaveCriticalSection(&mtx->section);
}
