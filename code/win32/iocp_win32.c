#pragma once

#define IOCTL_AFD_POLL 0x00012024

#define AFD_POLL_RECEIVE           0x0001
#define AFD_POLL_RECEIVE_EXPEDITED 0x0002
#define AFD_POLL_SEND              0x0004
#define AFD_POLL_DISCONNECT        0x0008
#define AFD_POLL_ABORT             0x0010
#define AFD_POLL_LOCAL_CLOSE       0x0020
#define AFD_POLL_ACCEPT            0x0080
#define AFD_POLL_CONNECT_FAIL      0x0100

#define READABLE_FLAGS     (AFD_POLL_RECEIVE | AFD_POLL_DISCONNECT | AFD_POLL_ACCEPT | AFD_POLL_ABORT | AFD_POLL_CONNECT_FAIL)
#define WRITABLE_FLAGS     (AFD_POLL_SEND | AFD_POLL_ABORT | AFD_POLL_CONNECT_FAIL)
#define ERROR_FLAGS        (AFD_POLL_CONNECT_FAIL)
#define READ_CLOSED_FLAGS  (AFD_POLL_DISCONNECT | AFD_POLL_ABORT | AFD_POLL_CONNECT_FAIL)
#define WRITE_CLOSED_FLAGS (AFD_POLL_DISCONNECT | AFD_POLL_CONNECT_FAIL)

#define ALWAYS_REPORTED_FLAGS (AFD_POLL_CONNECT_FAIL | AFD_POLL_ABORT)

#define FileReplaceCompletionInformation (61)

typedef LONG NTSTATUS;

typedef struct _FILE_COMPLETION_INFORMATION {
    HANDLE Port;
    PVOID  Key;
} FILE_COMPLETION_INFORMATION, *PFILE_COMPLETION_INFORMATION;

typedef struct _AFD_POLL_HANDLE_INFO {
    HANDLE Handle;
    ULONG Events;
    NTSTATUS Status;
} AFD_POLL_HANDLE_INFO, *LPAFD_POLL_HANDLE_INFO;

typedef struct _AFD_POLL_INFO {
    LARGE_INTEGER Timeout;
    ULONG NumberOfHandles;
    ULONG Exclusive;
    AFD_POLL_HANDLE_INFO Handles[1];
} AFD_POLL_INFO, *LPAFD_POLL_INFO;

typedef struct IoSourceState {
    size_t        ref;
    AFD_POLL_INFO params;
    OVERLAPPED    ovlp;
    uintptr_t     token;
} IoSourceState;

__kernel_entry NTSYSCALLAPI NTSTATUS NTAPI NtSetInformationFile(
    HANDLE                 FileHandle,
    PIO_STATUS_BLOCK       IoStatusBlock,
    PVOID                  FileInformation,
    ULONG                  Length,
    FILE_INFORMATION_CLASS FileInformationClass
);

IoSourceState* IoSourceState_create(void)
{
    IoSourceState *ret = malloc(sizeof(*ret));
    memset(ret, 0, sizeof(*ret));
    ret->ref = 1;
    return ret;
}

void IoSourceState_release(IoSourceState *state)
{
    if (--state->ref == 0) {
        free(state);
    }
}

IoSourceState* IoSourceState_from_overlapped(OVERLAPPED *ovlp)
{
    return CAST_STRUCT_FROM_MEMBER(ovlp, IoSourceState, ovlp);
}

int afd_poll(HANDLE handle, LPAFD_POLL_INFO params, LPOVERLAPPED ovlp)
{
    BOOL bResult = DeviceIoControl(
        handle,
        IOCTL_AFD_POLL,
        params,
        sizeof(*params),
        params,
        sizeof(*params),
        NULL,
        ovlp
    );

    if (bResult == FALSE) {
        DWORD LastError = GetLastError();
        if (LastError != ERROR_IO_PENDING) {
            log_error("DeviceIoControl failed on handle %p, err: %lu", handle, LastError);
            return 1;
        }
    }

    return 0;
}

int iocp_setup(Iocp *iocp)
{
    HANDLE hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    if (hIocp == NULL) {
        log_error("CreateIoCompletionPort failed %lu", GetLastError());
        return 1;
    }

    iocp->handle = (uintptr_t) hIocp;
    return 0;
}

void iocp_free(Iocp *iocp)
{
    if (iocp->handle != 0) {
        CloseHandle((HANDLE) iocp->handle);
        iocp->handle = 0;
    }
}

int iocp_register(Iocp *iocp, IoSource *source, uintptr_t token, int flags)
{
    source->state->token = token;

    HANDLE result = CreateIoCompletionPort(
        (HANDLE) source->socket,
        (HANDLE) iocp->handle,
        token,
        0);

    if (result != (HANDLE) iocp->handle) {
        log_error("Failed to register socket %" PRIXPTR "to iocp %" PRIXPTR, source->socket, iocp->handle);
        return 1;
    }

    return iocp_reregister(iocp, source, flags);
}

int iocp_reregister(Iocp *iocp, IoSource *source, int flags)
{
    UNREFERENCED_PARAMETER(iocp);

    IoSourceState *state = source->state;

    if (state->ref != 1) {
        log_error("Expected a single reference count, %zu found", state->ref);
        return 1;
    }

    ULONG events = 0;
    if ((flags & IOCPF_READ) != 0)
        events |= READABLE_FLAGS;
    if ((flags & IOCPF_WRITE) != 0)
        events |= WRITABLE_FLAGS;

    state->params.Timeout.QuadPart = INT64_MAX;
    state->params.NumberOfHandles = 1;
    state->params.Exclusive = 0;
    state->params.Handles[0].Handle = (HANDLE) source->socket;
    state->params.Handles[0].Events = ALWAYS_REPORTED_FLAGS | events;
    state->params.Handles[0].Status = 0;

    // Leak the `IoSourceState`, but will be "unleaked" when we poll an event
    // from the same handle.
    ++state->ref;
    OVERLAPPED *ovlp = &state->ovlp;

    if (afd_poll((HANDLE) source->socket, &state->params, ovlp) != 0) {
        IoSourceState_release(state);
        log_error("afd_poll failed");
        return 1;
    }

    return 0;
}

int iocp_deregister(Iocp *iocp, IoSource *source)
{
    UNREFERENCED_PARAMETER(iocp);

    if (source->state->ref != 1) {
        log_warn("Tried to de-register %" PRIXPTR ", during a pending io operation", source->socket);
        return ERROR_INVALID_PARAMETER;
    }

    FILE_COMPLETION_INFORMATION info = {0};

    IO_STATUS_BLOCK status = {0};
    NTSTATUS result = NtSetInformationFile(
        (HANDLE) source->socket,
        &status,
        &info,
        sizeof(info),
        FileReplaceCompletionInformation);

    if (result < 0) {
        return (int) RtlNtStatusToDosError(result);
    }

    return 0;
}

int iocp_poll(Iocp *iocp, ArrayEvent *events, uint32_t timeout_ms)
{
    OVERLAPPED_ENTRY entries[IOCP_POLL_BATCH_MAX_COUNT];

    ULONG removed;
    BOOL bSuccess = GetQueuedCompletionStatusEx(
        (HANDLE) iocp->handle,
        entries,
        ARRAY_SIZE(entries),
        &removed,
        timeout_ms,
        FALSE);

    if (bSuccess == FALSE) {
        DWORD last_error = GetLastError();
        if (last_error == WAIT_TIMEOUT) {
            return ERR_TIMEOUT;
        }

        log_error(
            "GetQueuedCompletionStatusEx failed (handle: %" PRIXPTR "), err: %lu",
            iocp->handle,
            last_error
        );
        return 1;
    }

    Event *buffer = array_push(events, removed);

    if (buffer == NULL) {
        for (ULONG idx = 0; idx < removed; ++idx) {
            IoSourceState *state = IoSourceState_from_overlapped(entries[idx].lpOverlapped);
            IoSourceState_release(state);
        }

        log_error("Failed to allocate %lu events", removed);
        return 1;
    }

    for (ULONG idx = 0; idx < removed; ++idx) {
        IoSourceState *state = IoSourceState_from_overlapped(entries[idx].lpOverlapped);
        buffer[idx].token = entries[idx].lpCompletionKey;
        ULONG flags = state->params.Handles[0].Events;

        buffer[idx].flags = 0;
        if ((flags & READABLE_FLAGS) != 0) {
            buffer[idx].flags |= IOCPF_READ;
        }

        if ((flags & WRITABLE_FLAGS) != 0) {
            buffer[idx].flags |= IOCPF_WRITE;
        }
        
        IoSourceState_release(state);
    }

    return 0;
}
