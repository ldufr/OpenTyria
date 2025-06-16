#pragma once

bool parse_ipv4(SocketAddrV4 *result, const char *input, size_t len)
{
    char buffer[IPV4_MAX_LENGTH];
    if (sizeof(buffer) <= len) {
        return false;
    }

    memcpy(buffer, input, len);
    buffer[len] = 0;

    size_t port_idx = len;
    for (size_t idx = len - 1; idx < len; --idx) {
        if (buffer[idx] == ':') {
            buffer[idx] = 0;
            port_idx = idx + 1;
            break;
        }
    }

    if (port_idx == len) {
        return false;
    }

    struct sockaddr_storage storage = {0};
    struct sockaddr_in *sockaddr = (struct sockaddr_in *)&storage;
    sockaddr->sin_family = AF_INET;

    if (!parse_u16(&result->port, &buffer[port_idx], len - port_idx, 10)) {
        return false;
    }

    if (inet_pton(sockaddr->sin_family, buffer, &sockaddr->sin_addr) != 1) {
        return false;
    }

    STATIC_ASSERT(sizeof(result->bytes) == sizeof(sockaddr->sin_addr.s_addr));
    memcpy(result->bytes, &sockaddr->sin_addr.s_addr, sizeof(result->bytes));
    return true;
}

bool parse_ipv6(SocketAddrV6 *result, const char *input, size_t len)
{
    char buffer[IPV6_MAX_LENGTH];
    if (sizeof(buffer) <= len) {
        return false;
    }

    memcpy(buffer, input, len);
    buffer[len] = 0;

    size_t port_idx = len;
    for (size_t idx = len - 1; idx < len; --idx) {
        if (buffer[idx] == ':') {
            buffer[idx] = 0;
            port_idx = idx + 1;
            break;
        }
    }

    if (port_idx == len) {
        return false;
    }

    size_t ip_len = port_idx - 1;
    if (ip_len <= 2 || buffer[0] != '[' || buffer[ip_len - 1] != ']') {
        return false;
    }

    buffer[--ip_len] = 0;

    struct sockaddr_storage storage = {0};
    struct sockaddr_in6 *sockaddr = (struct sockaddr_in6 *)&storage;
    sockaddr->sin6_family = AF_INET6;

    if (!parse_u16(&result->port, &buffer[port_idx], len - port_idx, 10)) {
        return false;
    }

    if (inet_pton(sockaddr->sin6_family, &buffer[1], &sockaddr->sin6_addr) != 1) {
        return false;
    }

    STATIC_ASSERT(sizeof(result->bytes) == sizeof(sockaddr->sin6_addr.s6_addr));
    memcpy(result->bytes, sockaddr->sin6_addr.s6_addr, sizeof(result->bytes));

    return true;
}

bool parse_addr(SocketAddr *result, const char *input, size_t len)
{
    if (parse_ipv4(&result->v4, input, len)) {
        result->af = AddressFamily_V4;
        return true;
    } else if (parse_ipv6(&result->v6, input, len)) {
        result->af = AddressFamily_V6;
        return true;
    } else {
        return false;
    }
}

void SocketAddr_FromSocketAddrStorage(SocketAddr *result, struct sockaddr_storage *storage)
{
    switch (storage->ss_family) {
    case AF_INET:
        result->af = AddressFamily_V4;
        result->v4.port = ntohs(((struct sockaddr_in *)storage)->sin_port);
        memcpy(result->v4.bytes, &((struct sockaddr_in *)storage)->sin_addr.s_addr, sizeof(result->v4.bytes));
        break;
    case AF_INET6:
        result->af = AddressFamily_V6;
        result->v6.port = ntohs(((struct sockaddr_in6 *)storage)->sin6_port);
        memcpy(result->v6.bytes, ((struct sockaddr_in6 *)storage)->sin6_addr.s6_addr, sizeof(result->v6.bytes));
        break;
    default:
        abort();
    }
}

void SocketAddr_WriteSocketAddrStorage(struct sockaddr *result, SocketAddr *addr)
{
    switch (addr->af)
    {
    case AddressFamily_V4:
        result->sa_family = AF_INET;
        ((struct sockaddr_in*)result)->sin_port = htons(addr->v4.port);
        memcpy(&((struct sockaddr_in*)result)->sin_addr.s_addr, addr->v4.bytes, sizeof(addr->v4.bytes));
        break;
    case AddressFamily_V6:
        result->sa_family = AF_INET6;
        ((struct sockaddr_in6*)result)->sin6_port = htons(addr->v4.port);
        memcpy(&((struct sockaddr_in6*)result)->sin6_addr.s6_addr, addr->v6.bytes, sizeof(addr->v6.bytes));
        break;
    default:
        abort();
    }
}

SocketAddr SocketAddr_LocalHostV4()
{
    SocketAddr result = {.af = AddressFamily_V4};
    result.v4.port = 6112;
    result.v4.bytes[0] = 127;
    result.v4.bytes[3] = 1;
    return result;
}

bool create_nonblocking_socket(uintptr_t *result, int family)
{
    int err;

    uintptr_t fd;
    if ((err = sys_socket(&fd, family, SOCK_STREAM, IPPROTO_TCP)) != 0) {
        log_error("Failed to create a socket, err: %d", err);
        return false;
    }

    if ((err = sys_enable_nonblocking(fd, true)) != 0) {
        sys_closesocket(fd);
        log_error("Failed set a socket to non-blocking, err: %d", err);
        return false;
    }

    *result = fd;
    return true;
}

bool snprint_sockaddr(char *buffer, size_t size, struct sockaddr *addr, size_t addrlen)
{
    void *ptr;
    uint16_t port;
    switch (addr->sa_family) {
    case AF_INET:
        if (addrlen < sizeof(struct sockaddr_in)) {
            return false;
        }
        ptr = &((struct sockaddr_in *)addr)->sin_addr;
        port = ntohs(((struct sockaddr_in *)addr)->sin_port);
        break;
    case AF_INET6:
        if (addrlen < sizeof(struct sockaddr_in6)) {
            return false;
        }
        ptr = &((struct sockaddr_in6 *)addr)->sin6_addr;
        port = ntohs(((struct sockaddr_in6 *)addr)->sin6_port);
        break;
    default:
        log_warn("Unknown address family %hu", addr->sa_family);
        return false;
    }

    if (inet_ntop(addr->sa_family, ptr, buffer, size) == NULL) {
        log_error(
            "Failed to stringify sockaddr (family: %hu), err: %d",
            addr->sa_family,
            sys_errno()
        );
        return false;
    }

    size_t len = strlen(buffer);
    if (snprintf(buffer + len, size - len, ":%hu", port) < (int)(size - len)) {
        return true;
    } else {
        return false;
    }
}
