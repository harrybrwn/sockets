# Sockets
A socket library inspired by and imitating the Go standard library.

## Example
```c
#include "sockets.h"

#define nil 0 // because Go is awsome... fight me
#define MAXBUF 1024

int main()
{
    int err;
    ssize_t n, reqlen;
    socket_t s;
    char recv[MAXBUF], send[MAXBUF];

    sprintf(send, "GET / HTTP/1.1\r\n"
                  "Accept: text/html\r\n"
                  "\r\n");
    reqlen = strlen(send) + 1;

    err = Dial(&s, S_TCP, "golang.org:80");
    if (err != nil) {
        error("could not connect socket");
    }

    n = Write(&s, send, reqlen);
    if (n != reqlen) {
        error("could not write to socket");
    }

    while ((n = Read(&s, recv, MAXBUF - 1)) > 0) {
        printf("%s", recv);
    }

    return Close(&s);
}
```
