#pragma once

IoSourceState* IoSourceState_create(void);
void IoSourceState_release(IoSourceState *state);

void IoSource_setup(IoSource *source, uintptr_t socket)
{
    source->socket = socket;
    source->state = IoSourceState_create();
}

void IoSource_free(IoSource *source)
{
    sys_closesocket(source->socket);
    IoSourceState_release(source->state);
}

IoSource IoSource_take(IoSource *source)
{
    IoSource result = *source;
    source->socket = 0;
    source->state = NULL;
    return result;
}

void IoSource_reset(IoSource *source)
{
    IoSource temp = IoSource_take(source);
    IoSource_free(&temp);
}