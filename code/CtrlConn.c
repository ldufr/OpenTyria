#pragma once

int CtrlConn_Setup(CtrlConn *conn)
{
    UNREFERENCED_PARAMETER(conn);
    return 0;
}

void CtrlConn_Free(CtrlConn *conn)
{
    IoSource_reset(&conn->source);
    array_free(&conn->incoming);
    array_free(&conn->outgoing);
    array_free(&conn->messages);
    memset(conn, 0, sizeof(*conn));
}

int CtrlConn_FlushOutgoingBuffer(CtrlConn *conn)
{
    int err;
    size_t bytes_sent;
    if ((err = sys_send(conn->source.socket, conn->outgoing.ptr, conn->outgoing.len, &bytes_sent)) != 0) {
        if (sys_would_block(err)) {
            conn->writable = false;
            return ERR_OK;
        }

        log_error("Failed to send %zu bytes, err: %d", bytes_sent, err);
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

int CtrlConn_UpdateWrite(CtrlConn *conn)
{
    int err = 0;
    if (conn->writable && conn->outgoing.len) {
        err = CtrlConn_FlushOutgoingBuffer(conn);
    }
    return err;
}

int CtrlConn_WriteMessage(CtrlConn *conn, CtrlMsg *msg, size_t size)
{
    int err;

    assert(msg->msg_id < ARRAY_SIZE(CTRL_MSG_FORMATS));
    MsgFormat format = CTRL_MSG_FORMATS[msg->msg_id];

    size_t size_before = array_size(&conn->outgoing);

    uint8_t *dst;
    if ((dst = array_push(&conn->outgoing, MSG_MAX_BUFFER_SIZE)) == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    size_t written;
    if ((err = pack_msg(format, &written, msg->buffer, size, dst, MSG_MAX_BUFFER_SIZE)) != 0) {
        array_shrink(&conn->outgoing, size_before);
        return err;
    }

    array_shrink(&conn->outgoing, size_before + written);
    return ERR_OK;
}

int CtrlConn_GetMessages(CtrlConn *conn)
{
    int err;
    uint16_t header;
    size_t total_consumed = 0;

    while (sizeof(header) <= (conn->incoming.len - total_consumed)) {
        const uint8_t *input = &conn->incoming.ptr[total_consumed];
        size_t size = conn->incoming.len - total_consumed;

        header = le16dec(input);
        if (ARRAY_SIZE(CTRL_MSG_FORMATS) <= header) {
            return ERR_UNSUCCESSFUL;
        }

        MsgFormat format = CTRL_MSG_FORMATS[header];

        size_t consumed;
        CtrlMsg *msg = malloc(sizeof(*msg));
        if ((err = unpack_msg(format, &consumed, input, size, msg->buffer, sizeof(msg->buffer))) != 0) {
            free(msg);

            if (err != ERR_NOT_ENOUGH_DATA) {
                log_warn("Received invalid message from client %04" PRIXPTR, conn->token);
                return err;
            }

            break;
        }

        array_add(&conn->messages, msg);
        total_consumed += consumed;
    }

    array_remove_range_ordered(&conn->incoming, 0, total_consumed);
    return 0;
}

void CtrlConn_CtrlPeerDisconnected(CtrlConn *conn)
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

int CtrlConn_ProcessEvent(CtrlConn *conn, Event event)
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

int CtrlConn_WriteHandshake(CtrlConn *conn)
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

int CtrlConn_NotifyServerReady(CtrlConn *conn)
{
    CtrlMsg msg = { CtrlMsgId_ServerReady };
    // msg.server_id = ;
    // msg.

    return CtrlConn_WriteMessage(conn, &msg, sizeof(msg.server_ready));
}

int CtrlConn_SendPlayerLeaved(CtrlConn *conn)
{
    CtrlMsg msg = { CtrlMsgId_PlayerLeaved };
    return CtrlConn_WriteMessage(conn, &msg, sizeof(msg.player_leaved));
}
