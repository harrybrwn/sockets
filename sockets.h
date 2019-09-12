#ifndef SOCKETS_H
#define SOCKETS_H

#include <netinet/in.h>

typedef enum s_error
{
    ALL_GOOD = 0,
    ERR_BAD_PROTOCOL,
    ERR_BAD_ADDRESS,
    ERR_HOST_RESOLVE,
    ERR_SOCK_OPTS,
    ERR_ADDRESS_BIND,
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

// Dial dials a socket for some protocol.
// The `endpoint` string should be in the form of "host:port" where host is the
// host name of the server being connected to.
//
// returns 0 on success, s_error for errors, and -1 for errors outside
// the scope of this library (see sys/socket.h).
int Dial(socket_t* sock, protocol_t proto, char* endpoint);

// Write will write `len` bytes to the socket and return the number of bytes written.
// returns -1 for errors.
ssize_t Write(socket_t* sock, const void* buf, size_t len);

// Read reads data from a socket and puts it into a buffer.
// Will read `len` bytes into `buf` and return the number of bytes read.
// returns -1 for errors.
ssize_t Read(socket_t* sock, void* buf, size_t len);

// Listen initializes a socket, binds the address and listens for requests.
// The `endpoint` string should be in the form "address:port" where address is an ip
// address and NOT the host url.
//
// returns 0 on success, s_error for errors, and -1 for errors outside
// the scope of this library (see sys/socket.h).
int Listen(socket_t* sock, protocol_t proto, char* endpoint);

int resolve_hosturl(char*, char buf[INET_ADDRSTRLEN]);

#endif /* SOCKETS_H */