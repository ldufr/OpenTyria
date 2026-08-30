#pragma once

typedef enum AddressFamily {
    AddressFamily_None,
    AddressFamily_V4,
    AddressFamily_V6,
} AddressFamily;

typedef struct SocketAddrV4 {
    uint16_t port;
    uint8_t  bytes[4];
} SocketAddrV4;

typedef struct SocketAddrV6 {
    uint16_t port;
    uint8_t  bytes[16];
} SocketAddrV6;

typedef struct SocketAddr {
    AddressFamily    af;
    union
    {
        SocketAddrV4 v4;
        SocketAddrV6 v6;
    };
} SocketAddr;

bool parse_ipv4(SocketAddrV4 *result, const char *input, size_t len);
bool parse_ipv6(SocketAddrV6 *result, const char *input, size_t len);
bool parse_addr(SocketAddr *result, const char *input, size_t len);

void SocketAddr_FromSocketAddrStorage(SocketAddr *result, struct sockaddr_storage *storage);
void SocketAddr_WriteSocketAddrStorage(struct sockaddr *result, SocketAddr *addr);
SocketAddr SocketAddr_LocalHostV4();

bool create_nonblocking_socket(uintptr_t *result, int family);
bool snprint_sockaddr(char *buffer, size_t size, struct sockaddr *addr, size_t addrlen);
