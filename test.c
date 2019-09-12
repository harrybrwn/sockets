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
#define MAXBUF 1024

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

#define nil 0

int cool()
{
    int n, err;
    socket_t s;
    char sendline[MAXBUF], recvline[MAXBUF];

    sprintf(sendline, GET_DATA);
    memset(recvline, 0, MAXBUF);

    // this has a weird extra response at the end bc it is encoded in chunks
    // google chunked transfer-encoding.
    err = Dial(&s, S_TCP, "www.google.com:80");
    if (err != nil) // hehe... because Go is the best :)
        error("could not connect socket");

    if (Write(&s, sendline, sizeof(get_req)) != sizeof(get_req))
        error("could not write to socket");

    while ((n = Read(&s, recvline, MAXBUF - 1)) > 0)
        printf("%s", recvline);

    if (n < 0)
        error("problem reading from socket");

    Close(&s);
    return 0;
}

int example()
{
    int err;
    ssize_t n, reqlen;
    socket_t s;
    char recv[MAXBUF], send[MAXBUF];

    sprintf(send, "GET / HTTP/1.1\r\n"
                  "accept: text/html\r\n"
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

#undef for

#ifndef assert
#define assert(COND)                                                                              \
    if (!(COND)) {                                                                                \
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

#define FREE_SOCK             \
    free(s.endpoint->address);\
    free(s.endpoint);
int test_dial()
{
    socket_t s;
    assert(Dial(&s, 23, "www.google.com:80") == ERR_BAD_PROTOCOL);

    assert(Dial(&s, S_TCP, "") == ERR_BAD_ADDRESS);
    FREE_SOCK
    assert(Dial(&s, S_TCP, "www.google.com:") == ERR_BAD_ADDRESS);
    FREE_SOCK
    assert(Dial(&s, S_TCP, "www.google.com") == ERR_BAD_ADDRESS);
    FREE_SOCK

    bzero(s.endpoint, sizeof(struct s_endpoint));
    assert(Dial(&s, S_TCP, "www.google.com:80") == 0);
    Close(&s);
    return 0;
}

#include <ifaddrs.h>
#include <netdb.h>

static char* find_host_ip() {
    char* ip = malloc(MAXBUF);
    struct ifaddrs *ifaddr, *ifa;
    bzero(ip, MAXBUF);
    if (getifaddrs(&ifaddr) == -1)
        error("Error: getifaddrs");
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_INET && ifa->ifa_flags == 69699) {
            if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), ip, MAXBUF,NULL, 0, 1) != 0)
                error("could not get name info");
            break;
        }
    }
    freeifaddrs(ifaddr);
    return ip;
}

int test_listen()
{
    char* ip = find_host_ip();
    int i = strlen(ip);
    socket_t s;

    assert(Listen(&s, S_TCP, ":8080") == 0);
    Close(&s);
    assert(Listen(&s, S_TCP, "") == ERR_BAD_ADDRESS);
    FREE_SOCK
    assert(Listen(&s, S_TCP, "257.0.0.1:8888") == ERR_BAD_ADDRESS);
    Close(&s);

    ip[i] = ':';
    ip[i+1] = '8'; ip[i+2] = '0';
    ip[i+3] = '8'; ip[i+4] = '0';
    ip[i+5] = '\0';
    assert(Listen(&s, S_TCP, ip) == 0);
    assert(s.endpoint->port == 8080);
    Close(&s);
    ip[i] = 0;
    assert(Listen(&s, S_TCP, ip) == ERR_BAD_ADDRESS);
    Close(&s);
    ip[i] = ':'; ip[i+1] = 0;
    assert(Listen(&s, S_TCP, ip) == ERR_BAD_ADDRESS);
    Close(&s);

    free(ip);
    return 0;
}
#undef FREE_SOCK

#define START_TESTS int res = 0
#define END_TESTS  \
End:               \
    return res
#define RUN_TEST(FN)       \
    if ((res = FN()) != 0) \
        goto End;

int main()
{
    START_TESTS;

    RUN_TEST(test_parse_endpoint);
    RUN_TEST(test_port_atoi);
    RUN_TEST(test_grab_header_key);
    RUN_TEST(test_listen);
    RUN_TEST(test_dial); // does not work with bad internet

    END_TESTS;

    // return basic();
    // return cool();
    // return example();
}