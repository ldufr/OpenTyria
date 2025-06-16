#pragma once

#define IOCPF_READ  (1)
#define IOCPF_WRITE (2)

#define IOCP_POLL_BATCH_MAX_COUNT (256)

typedef struct IoSourceState IoSourceState;

typedef struct Event {
    uintptr_t token;
    uint32_t  flags;
} Event;

typedef array(Event) ArrayEvent;

typedef struct IoSource {
    uintptr_t      socket;
    IoSourceState *state;
} IoSource;

typedef array(IoSource) ArrayIoSource;

void IoSource_setup(IoSource *source, uintptr_t socket);
void IoSource_free(IoSource *source);
IoSource IoSource_take(IoSource *source);
void IoSource_reset(IoSource *source);

typedef struct Iocp {
    uintptr_t handle;
} Iocp;

int  iocp_setup(Iocp *iocp);
void iocp_free(Iocp *iocp);

int iocp_register(Iocp *iocp, IoSource *source, uintptr_t token, int flags);
int iocp_reregister(Iocp *iocp, IoSource *source, int flags);
int iocp_deregister(Iocp *iocp, IoSource *source);
int iocp_poll(Iocp *iocp, ArrayEvent *events, uint32_t timeout_ms);
