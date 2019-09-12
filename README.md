# Sockets
A socket library inspired by the Go standard library.

## Example
```c
#include "socktes.h"

#define nil 0 // because Go is awsome... fight me
#define MAXBUF 1024

int main()
{
    int err;
    ssize_t n, reqlen;
    socket_t s;
    char recv[MAXBUF], send[MAXBUF];

    err = Dial(&s, S_TCP, "golang.org:80");
    if (err != nil)
        error("could not connect socket");

    sprintf(send, "GET / HTTP/1.1\r\n"
                  "accept: text/html\r\n"
                  "\r\n");
    reqlen = sizeof(send) + 1;
    n = Write(&s, send, MAXBUF);
    if (n != reqlen)
        error("could not write to socket");

    while ((n = Read(&s, recv, MAXBUF - 1)))
        printf("%s", recv);

    return Close(&s);
}
```