#ifndef SOCKETS_H
#define SOCKETS_H

#include <netinet/in.h>

typedef enum s_error
{
    ALL_GOOD = 0,
    ERR_BAD_PROTOCOL,
    ERR_BAD_ADDRESS,
    ERR_HOST_RESOLVE,
} s_error;

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

// Close closes the socket.
int Close(socket_t*);

// s_dial dials a socket for some protocol.
// returns 0 on success, s_error for errors, and -1 for errors outside
// the scope of this library.
int Dial(socket_t*, protocol_t, char* endpoint);

// s_write will write some data to the socket.
ssize_t Write(socket_t* sock, const void* buf, size_t len);

ssize_t Read(socket_t* sock, void* buf, size_t len);

// s_listen initializes a socket, binds the address and listens for requests.
int s_listen();

int resolve_hosturl(char*, char buf[INET_ADDRSTRLEN]);

#endif /* SOCKETS_H */