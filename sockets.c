#include "sockets.h"
#include "util/map.h"
#include <stdio.h>
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

static int parse_endpoint(char*, struct s_endpoint*);
static int s_connect(socket_t* sock);

int s_close(socket_t* sock)
{
    free(sock->endpoint->address);
    free(sock->endpoint);
    return close(sock->_fd);
}

int s_dial(socket_t* sock, protocol_t proto, char* endpoint)
{
    char ip_str[INET_ADDRSTRLEN];

    if (proto == S_TCP)
    {
        INET_SOCKET_INIT(sock, SOCK_STREAM, IPPROTO_TCP);
    }
    else if (proto == S_UDP)
    {
        INET_SOCKET_INIT(sock, SOCK_DGRAM, IPPROTO_UDP);
    }
    else
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
    return s_connect(sock);
}

static int s_connect(socket_t* sock)
{
    return connect(sock->_fd, (struct sockaddr*)&sock->_addr, sizeof(struct sockaddr));
}

ssize_t s_write(socket_t* sock, const void* buf, size_t len)
{
    return write(sock->_fd, buf, len);
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

    while (ip[i])
    {
        buf[i] = ip[i];
        i++;
    }
    buf[i] = 0;
    return 0;
}

static uint16_t port_atoi(char str[6])
{
    char c;
    uint16_t n = 0;

    if (*str == '\0')
        return 0;

    while ((c = *str++))
    {
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
    while (1)
    {
        c = *raw++;
        switch (c)
        {
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
    while ((c = *raw++))
        port[i++] = c;
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
    for (i = 0; (c = *raw++);)
    {
        if (c == ':')
            break;
        buf[i++] = c;
    }
    buf[i++] = 0;

    key = malloc(i);
    strncpy(key, buf, i);
    return key;
}