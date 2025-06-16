#pragma once

#define READABLE_FLAGS (EPOLLET | EPOLLIN | EPOLLRDHUP)
#define WRITABLE_FLAGS (EPOLLET | EPOLLOUT)

typedef struct IoSourceState {
    uintptr_t token;
} IoSourceState;

IoSourceState* IoSourceState_create(void)
{
    IoSourceState *ret = malloc(sizeof(*ret));
    memset(ret, 0, sizeof(*ret));
    return ret;
}

void IoSourceState_release(IoSourceState *state)
{
    free(state);
}

int iocp_setup(Iocp *iocp)
{
    int efd;
    if ((efd = epoll_create1(EPOLL_CLOEXEC)) == -1) {
        log_error("epoll_create1 failed %d", sys_errno());
        return 1;
    }

    iocp->handle = (uintptr_t) efd;
    return 0;
}

void iocp_free(Iocp *iocp)
{
    if (iocp->handle != 0) {
        close((int) iocp->handle);
        iocp->handle = 0;
    }
}

int epoll_register(int efd, int op, int fd, uintptr_t token, int flags)
{
    struct epoll_event event;
    event.data.u64 = (uint64_t)token;
    event.events = EPOLLONESHOT;

    if ((flags & IOCPF_READ) != 0)
        event.events |= READABLE_FLAGS;
    if ((flags & IOCPF_WRITE) != 0)
        event.events |= WRITABLE_FLAGS;

    return epoll_ctl(efd, op, fd, &event);
}

int iocp_register(Iocp *iocp, IoSource *source, uintptr_t token, int flags)
{
    source->state->token = token;

    int efd = (int) iocp->handle;
    int fd = (int) source->socket;
    if (epoll_register(efd, EPOLL_CTL_ADD, fd, token, flags) != 0) {
        log_error("epoll_register failed, err: %d", sys_errno());
        return ERR_UNSUCCESSFUL;
    }

    return 0;
}

int iocp_reregister(Iocp *iocp, IoSource *source, int flags)
{
    IoSourceState *state = source->state;

    int efd = (int) iocp->handle;
    int fd = (int) source->socket;
    if (epoll_register(efd, EPOLL_CTL_MOD, fd, source->state->token, flags) != 0) {
        log_error("epoll_register failed, err: %d", sys_errno());
        return ERR_UNSUCCESSFUL;
    }

    return 0;
}

int iocp_deregister(Iocp *iocp, IoSource *source)
{
    UNREFERENCED_PARAMETER(iocp);

    // This param is ignored, but on some version of the kernel, we still needed
    // to pass a pointer to this struct.
    struct epoll_event event = {0};

    int efd = (int) iocp->handle;
    int fd = (int) source->socket;
    if (epoll_ctl(efd, EPOLL_CTL_DEL, fd, &event) != 0) {
        log_error("epoll_ctl(EPOLL_CTL_DEL) failed, err: %d", sys_errno());
        return ERR_UNSUCCESSFUL;
    }

    return 0;
}

int iocp_poll(Iocp *iocp, ArrayEvent *events, uint32_t timeout_ms)
{
    int err;
    struct epoll_event entries[IOCP_POLL_BATCH_MAX_COUNT];

    int timeout;
    if (timeout_ms == UINT32_MAX) {
        timeout = -1;
    } else if (UINT32_MAX <= INT_MAX) {
        timeout = (int) timeout_ms;
    } else {
        timeout = (int)u32min(INT_MAX, timeout_ms);
    }

    int efd = (int) iocp->handle;
    if ((err = epoll_wait(efd, entries, ARRAY_SIZE(entries), timeout)) < 0) {
        err = sys_errno();
        if (err == EINTR) {
            return ERR_TIMEOUT;
        }
        log_error("epoll_wait failed, err: %d", err);
        return ERR_UNSUCCESSFUL;
    } else if (err == 0) {
        return ERR_TIMEOUT;
    }

    size_t removed = (size_t) err;
    Event *buffer = array_push(events, removed);
    if (buffer == NULL) {
        log_error("Failed to allocate %lu events", removed);
        return ERR_OUT_OF_MEMORY;
    }

    for (size_t idx = 0; idx < removed; ++idx) {
        buffer[idx].token = (uintptr_t) entries[idx].data.u64;
        buffer[idx].flags = 0;

        if ((entries[idx].events & READABLE_FLAGS) != 0) {
            buffer[idx].flags |= IOCPF_READ;
        }
        if ((entries[idx].events & WRITABLE_FLAGS) != 0) {
            buffer[idx].flags |= IOCPF_WRITE;
        }
    }

    return 0;
}
