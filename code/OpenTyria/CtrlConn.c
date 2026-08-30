#pragma once

int CtrlConn_Setup(CtrlConnection *conn)
{
    UNREFERENCED_PARAMETER(conn);
    return 0;
}

void CtrlConn_Free(CtrlConnection *conn)
{
    IoSource_reset(&conn->source);
    array_free(&conn->incoming);
    array_free(&conn->outgoing);
    array_free(&conn->messages);
    memset(conn, 0, sizeof(*conn));
}

int CtrlConn_FlushOutgoingBuffer(CtrlConnection *conn)
{
    int err;
    size_t bytes_sent;
    if ((err = sys_send(conn->source.socket, conn->outgoing.ptr, conn->outgoing.len, &bytes_sent)) != 0) {
        if (sys_would_block(err)) {
            conn->writable = false;
            return ERR_OK;
        }

        log_error("Failed to send %zu bytes, err: %d", conn->outgoing.len, err);
        return ERR_UNSUCCESSFUL;
    }

    if (bytes_sent != conn->outgoing.len) {
        conn->writable = false;
        array_remove_range_ordered(&conn->outgoing, 0, bytes_sent);
    } else {
        array_clear(&conn->outgoing);
    }

    return ERR_OK;
}

int CtrlConn_UpdateWrite(CtrlConnection *conn)
{
    int err = 0;
    if (conn->writable && conn->outgoing.len) {
        err = CtrlConn_FlushOutgoingBuffer(conn);
    }
    return err;
}

size_t CtrlConn_SizeOf(CtrlMsgId msg_id)
{
    switch (msg_id) {
#define X(name) case CtrlMsgId_ ## name: return sizeof(CtrlMsg_ ## name);
    CtrlMsgDefs
#undef X
    default:
        abort();
    }
}

int CtrlConn_WriteMessage(CtrlConnection *conn, CtrlMsg *msg, size_t size)
{
    assert(CtrlConn_SizeOf(msg->msg_id) == size);

    uint8_t *dst;
    if ((dst = array_push(&conn->outgoing, size)) == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    memcpy(dst, msg, size);
    return ERR_OK;
}

int CtrlConn_GetMessages(CtrlConnection *conn)
{
    uint16_t header;
    size_t total_consumed = 0;

    while (sizeof(header) <= (conn->incoming.len - total_consumed)) {
        const uint8_t *input = &conn->incoming.ptr[total_consumed];
        size_t size = conn->incoming.len - total_consumed;

        memcpy(&header, input, sizeof(header));
        if (CtrlMsgId_Count <= header) {
            return ERR_UNSUCCESSFUL;
        }

        size_t msg_size = CtrlConn_SizeOf((CtrlMsgId) header);
        if (size < msg_size) {
            break;
        }

        CtrlMsg *msg = malloc(sizeof(*msg));
        memcpy(msg, input, msg_size);

        array_add(&conn->messages, msg);
        total_consumed += msg_size;
    }

    array_remove_range_ordered(&conn->incoming, 0, total_consumed);
    return 0;
}

void CtrlConn_CtrlPeerDisconnected(CtrlConnection *conn)
{
    int err;

    size_t count = array_size(&conn->messages);
    if (count != 0 && array_at(&conn->messages, count - 1) == NULL) {
        log_warn("Tried to peer disconnect twice");
        return;
    }

    if ((err = CtrlConn_GetMessages(conn)) != 0) {
        // @Cleanup: Figure out what to do...
        log_error("CtrlConn_GetMessages failed %d", err);
    }

    array_add(&conn->messages, NULL);
    IoSource_reset(&conn->source);
    array_clear(&conn->outgoing);
}

int CtrlConn_ProcessEvent(CtrlConnection *conn, Event event)
{
    int err;

    if ((event.flags & IOCPF_WRITE) != 0) {
        conn->writable = true;
    }

    if ((event.flags & IOCPF_READ) != 0) {
        for (;;) {
            uint8_t *buffer;

            const size_t size_at_starts = conn->incoming.len;
            if ((buffer = array_push(&conn->incoming, AUTH_READ_CHUNK_SIZE)) == NULL) {
                log_error("Out of memory will reading from socket");
                break;
            }

            size_t bytes;
            if ((err = sys_recv(conn->source.socket, buffer, AUTH_READ_CHUNK_SIZE, &bytes)) != 0) {
                array_shrink(&conn->incoming, size_at_starts);
                if (!sys_would_block(err)) {
                    log_error(
                        "Error while reading %04" PRIXPTR ", err: %d",
                        conn->source.socket,
                        err
                    );

                    CtrlConn_CtrlPeerDisconnected(conn);
                }

                break;
            }

            array_shrink(&conn->incoming, size_at_starts + bytes);
            if (bytes == 0) {
                CtrlConn_CtrlPeerDisconnected(conn);
                break;
            }
        }

        if ((err = CtrlConn_GetMessages(conn)) != 0) {
            log_error("CtrlConn_GetMessages failed, err: %d", err);
        }
    }

    return 0;
}

int CtrlConn_WriteHandshake(CtrlConnection *conn)
{
    CTRL_CMSG_VERSION version = {0};
    version.header = CTRL_CMSG_VERSION_HEADER;

    uint8_t *buffer;
    if ((buffer = array_push(&conn->outgoing, sizeof(version))) == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    memcpy(buffer, &version, sizeof(version));
    return 0;
}

int CtrlConn_SendServerReady(CtrlConnection *conn, uint32_t server_id)
{
    CtrlMsg msg = { CtrlMsgId_ServerReady };
    msg.ServerReady.server_id = server_id;
    return CtrlConn_WriteMessage(conn, &msg, sizeof(msg.ServerReady));
}

int CtrlConn_SendPlayerLeft(CtrlConnection *conn, GmUuid account_id, GmUuid char_id)
{
    CtrlMsg msg = { CtrlMsgId_PlayerLeft };
    msg.PlayerLeft.account_id = account_id;
    msg.PlayerLeft.char_id = char_id;
    return CtrlConn_WriteMessage(conn, &msg, sizeof(msg.PlayerLeft));
}
