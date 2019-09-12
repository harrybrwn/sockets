#ifndef SOCKETS_H
#define SOCKETS_H

#include <netinet/in.h>

// s_endpoint is represents an address/port pair, think of it as being
// www.address.com:port format
struct s_endpoint
{
    uint16_t port;
    char* address;
};

typedef struct socket
{
    int _fd;
    struct sockaddr_in _addr;

    struct s_endpoint* endpoint;
} socket_t;

typedef enum protocol_type
{
    S_TCP = 0,
    S_UDP,
} protocol_t;

// s_close closes the socket.
int s_close(socket_t*);

// s_dial dials a socket for some protocol.
// returns 0 on success, -1 for errors.
int s_dial(socket_t*, protocol_t, char* endpoint);

// s_write will write some data to the socket.
size_t s_write(socket_t* sock, const void* buf, size_t len);

// s_listen initializes a socket, binds the address and listens for requests.
int s_listen();

int resolve_hosturl(char*, char buf[INET_ADDRSTRLEN]);

#endif /* SOCKETS_H */