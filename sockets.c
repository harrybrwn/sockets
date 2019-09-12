#include "sockets.h"
#include <stdlib.h>
#include <string.h>

// sockets
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define INET_SOCKET_INIT(SOCK, TYPE, PROTO)                                                       \
    SOCK->_fd = socket(AF_INET, TYPE, PROTO);                                                     \
    bzero(&SOCK->_addr, sizeof(SOCK->_addr));                                                     \
    SOCK->_addr.sin_family = AF_INET;

int Close(socket_t* sock)
{
    free(sock->endpoint->address);
    free(sock->endpoint);
    return close(sock->_fd);
}

static int parse_endpoint(char*, struct s_endpoint*);
static inline int init_socket(socket_t*, protocol_t);

int Dial(socket_t* sock, protocol_t proto, char* endpoint)
{
    char ip_str[INET_ADDRSTRLEN];

    if (init_socket(sock, proto) == -1)
        return ERR_BAD_PROTOCOL;

    sock->endpoint = malloc(sizeof(struct s_endpoint));
    sock->endpoint->address = malloc(strlen(endpoint) + 1);
    if (parse_endpoint(endpoint, sock->endpoint) != 0)
        return ERR_BAD_ADDRESS;

    sock->_addr.sin_port = htons(sock->endpoint->port);

    if (resolve_hosturl(sock->endpoint->address, ip_str) != 0)
        return ERR_HOST_RESOLVE;
    if (inet_pton(AF_INET, ip_str, &sock->_addr.sin_addr) != 1)
        return -1;
    return connect(sock->_fd, (struct sockaddr*)&sock->_addr, sizeof(struct sockaddr));
}

#define sa struct sockaddr

int Listen(socket_t* sock, protocol_t proto, char* endpoint)
{
    int enable_opt = 1;

    if (init_socket(sock, proto) == -1)
        return ERR_BAD_PROTOCOL;

    sock->endpoint = malloc(sizeof(struct s_endpoint));
    sock->endpoint->address = malloc(strlen(endpoint) + 1);
    if (parse_endpoint(endpoint, sock->endpoint) != 0)
        return ERR_BAD_ADDRESS;

    sock->_addr.sin_port = htons(sock->endpoint->port);
    if (endpoint[0] == ':') {
        sock->_addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, sock->endpoint->address, &sock->_addr.sin_addr) != 1)
            return ERR_BAD_ADDRESS;
    }

    if (setsockopt(sock->_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &enable_opt, sizeof(int)) != 0) {
        return ERR_SOCK_OPTS;
    }
    if (bind(sock->_fd, (sa*)&sock->_addr, sizeof(sa)) != 0) {
        return ERR_ADDRESS_BIND;
    }

    return listen(sock->_fd, 5);
}

ssize_t Write(socket_t* sock, const void* buf, size_t len)
{
    return write(sock->_fd, buf, len);
}

ssize_t Read(socket_t* sock, void* buf, size_t len)
{
    return read(sock->_fd, buf, len);
}

int resolve_hosturl(char* hosturl, char buf[INET_ADDRSTRLEN])
{
    int i = 0;
    struct in_addr** addr;
    struct hostent* host = gethostbyname(hosturl);
    if (host == NULL)
        return -1;

    addr = (struct in_addr**)host->h_addr_list;
    char* ip = inet_ntoa(*addr[0]);

    while (ip[i]) {
        buf[i] = ip[i];
        i++;
    }
    buf[i] = 0;
    return 0;
}

static inline int init_socket(socket_t* s, protocol_t proto)
{
    if (proto == S_TCP) {
        INET_SOCKET_INIT(s, SOCK_STREAM, IPPROTO_TCP);
    }
    else if (proto == S_UDP) {
        INET_SOCKET_INIT(s, SOCK_DGRAM, IPPROTO_UDP);
    }
    else
        return -1;
    return 0;
}

static uint16_t port_atoi(char str[6])
{
    char c;
    uint16_t n = 0;

    if (*str == '\0')
        return 0;

    while ((c = *str++)) {
        if (c < '0' || c > '9')
            return 0;
        n = (n << 3) + (n << 1) + c - '0';
    }
    return n;
}

// TODO: add dumy checks here and return values based on user error. like
// check if `raw` if actually a url or that it has a port number.
static int parse_endpoint(char* raw, struct s_endpoint* endpoint)
{
    char port[6]; // space for biggest port (65535) and '\0'

    char c;
    int i = 0;
    while (1) {
        c = *raw++;
        switch (c) {
        case ':':
            goto next;
        case '\0':
            return -1;
        default:
            endpoint->address[i++] = c;
        }
    }
next:

    i = 0;
    while ((c = *raw++)) {
        port[i++] = c;
    }
    port[i] = 0;
    if (i == 0)
        return -1;

    endpoint->port = port_atoi(port);
    return 0;
}

static char* grab_header_key(char** data)
{
    char buf[512], *key, c;
    int i;

    char* raw = *data;
    for (i = 0; (c = *raw++);) {
        if (c == ':')
            break;
        buf[i++] = c;
    }
    buf[i++] = 0;

    key = malloc(i);
    strncpy(key, buf, i);
    return key;
}