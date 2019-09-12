#include "sockets.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// sockets
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 80
#define MAXLINE 1024
#define SA struct sockaddr

#define ERR                                                                                       \
    printf("(%s:%d) error!\n", __FILE__, __LINE__);                                               \
    exit(1)

char url[] = "www.google.com";
#define GET_DATA                                                                                  \
    "GET / HTTP/1.1\r\n"                                                                          \
    "accept: text/html\r\n"                                                                       \
    "\r\n"
char get_req[] = GET_DATA;

void error(char* msg)
{
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
}

int basic()
{
    int socketfd, n;
    int sendbytes;
    struct sockaddr_in servaddr;
    char sendline[MAXLINE];
    char recvline[MAXLINE];
    char ip[INET_ADDRSTRLEN];

    /* data stuff (not important) */
    sprintf(sendline, GET_DATA);
    sendbytes = strlen(sendline);
    memset(recvline, 0, MAXLINE);
    // ---------------------

    /* Create Socket */
    socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketfd < 0)
    {
        ERR;
    }

    /* Address Init */
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_port = htons(PORT);
    servaddr.sin_family = AF_INET;

    resolve_hosturl(url, ip);
    printf("%s\n", ip);
    if (inet_pton(AF_INET, ip, &servaddr.sin_addr) < 0)
    {
        ERR;
    }
    printf("address is www.google.com: %s\n", ip);

    /* Connect to Server at servaddr */
    int res;
    if ((res = connect(socketfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) < 0)
    {
        printf("got %d\n", res);
        ERR;
    }

    /* Write the message to the Socket
        - returns number of bytes written
    */
    if (write(socketfd, sendline, sendbytes) != sendbytes)
    {
        ERR;
    }

    /* Read data sent by the server
        - if read returns -1 there was an error
        - if read returns 0 then you have reached the end of the data */
    while ((n = read(socketfd, recvline, MAXLINE - 1)) > 0)
    {
        printf("%d\n", n);
        printf("%s\n", recvline);
    }
    printf("\n");

    if (n < 0)
    {
        ERR;
    }

    close(socketfd);
    return 0;
}

#define nil 0

int cool()
{
    int n, err;
    socket_t s;
    char sendline[MAXLINE], recvline[MAXLINE];

    sprintf(sendline, GET_DATA);
    memset(recvline, 0, MAXLINE);

    // this has a weird extra response at the end bc it is encoded in chunks
    // google chunked transfer-encoding.
    err = Dial(&s, S_TCP, "www.google.com:80");
    if (err != nil) // hehe... because Go is the best :)
        error("could not connect socket");

    if (Write(&s, sendline, sizeof(get_req)) != sizeof(get_req))
        error("could not write to socket");

    while ((n = Read(&s, recvline, MAXLINE - 1)) > 0)
        printf("%s", recvline);

    if (n < 0)
        error("problem reading from socket");

    Close(&s);
    return 0;
}

#define MAXBUF 1024

int example()
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

#ifndef assert
#define assert(COND)                                                                              \
    if (!(COND))                                                                                  \
    {                                                                                             \
        printf("\033[0;31mAssertion failed: %s:%d\033[0m \"%s\"\n", __FILE__, __LINE__, #COND);   \
        return 1;                                                                                 \
    }
#endif

#define assert_str_eq(STR1, STR2) assert(strcmp(STR1, STR2) == 0)
#define assert_strn_eq(s1, s2, n) assert(strncmp(s1, s2, n) == 0)

int test_parse_endpoint()
{
    char* urls[] = { "www.google.com:80", "golang.org:443", ":8080" };
    struct s_endpoint ep;
    ep.address = malloc(256);

    assert(parse_endpoint(urls[0], &ep) == 0);
    assert_strn_eq(ep.address, "www.google.com", 14);
    assert(ep.port == 80);
    bzero(ep.address, 256);

    assert(parse_endpoint(urls[1], &ep) == 0);
    assert_strn_eq(ep.address, "golang.org", 10);
    assert(ep.port == 443);
    bzero(ep.address, 256);

    assert(parse_endpoint(urls[2], &ep) == 0);
    assert_str_eq(ep.address, "");
    assert(ep.port == 8080);

    assert(parse_endpoint("portlesshost", &ep) == -1);
    assert(parse_endpoint("colonbutnoport:", &ep) == -1);

    free(ep.address);
    return 0;
}

int test_port_atoi()
{
    assert(port_atoi("65535") == 65535);
    assert(port_atoi("1") == 1);
    assert(port_atoi("0") == 0);
    assert(port_atoi("8080") == 8080);
    assert(port_atoi("9876") == 9876);
    assert(port_atoi("54321") == 54321);
    assert(port_atoi("65536") == 0); // 65,536 is bigger than uint16_t
    assert(port_atoi("w") == 0);
    return 0;
}

int test_grab_header_key()
{
    char* line;
    char* key;

    line = "one: two";
    key = grab_header_key(&line);
    assert_str_eq(key, "one");
    free(key);

    line = "accept: that\r\n";
    key = grab_header_key(&line);
    assert_str_eq(key, "accept");
    free(key);

    line = " weird-HEAD+er:       va()\\lue";
    key = grab_header_key(&line);
    assert_str_eq(key, " weird-HEAD+er");
    free(key);

    return 0;
}

int test_dial()
{
    socket_t s;
    assert(Dial(&s, 23, "www.google.com:80") == -1);

    assert(Dial(&s, S_TCP, "") == -1);
    free(s.endpoint->address);
    free(s.endpoint);
    assert(Dial(&s, S_TCP, "www.google.com:") == -1);
    free(s.endpoint->address);
    free(s.endpoint);
    assert(Dial(&s, S_TCP, "www.google.com") == -1);
    free(s.endpoint->address);
    free(s.endpoint);
    bzero(s.endpoint, sizeof(struct s_endpoint));
    assert(Dial(&s, S_TCP, "www.google.com:443") == 0);
    Close(&s);
    return 0;
}

#define START_TESTS int res;
#define RUN_TEST(FN)                                                                              \
    if ((res = FN()) != 0)                                                                        \
        return res;

int main()
{
    START_TESTS;
    RUN_TEST(test_parse_endpoint);
    RUN_TEST(test_port_atoi);
    RUN_TEST(test_grab_header_key);
    // RUN_TEST(test_dial);

    // return basic();
    return cool();
    // return example();
}