#include "sockets.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// sockets
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAXBUF 1024
#define nil 0
#define GET_DATA                                                                                  \
    "GET / HTTP/1.1\r\n"                                                                          \
    "accept: text/html\r\n"                                                                       \
    "\r\n"

void error(char* msg)
{
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
}

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

    if (Write(&s, sendline, sizeof(GET_DATA)) != sizeof(GET_DATA))
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
    ssize_t n, reqlen, total = 0;
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
        printf("%s\n", recv);
        total += n;
    }
    if (n == -1)
        error("could not read socket data");

    printf("read a total of %ld bytes\n", total);
    int ret = Close(&s);
    printf("%d\n", ret);
    return ret;
}

#include <ifaddrs.h>
#include <netdb.h>

static char* find_host_ip()
{
    char* ip = malloc(MAXBUF);
    struct ifaddrs *ifaddr, *ifa;
    bzero(ip, MAXBUF);
    if (getifaddrs(&ifaddr) == -1)
        error("Error: getifaddrs");
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_INET && ifa->ifa_flags == 69699) {
            if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), ip, MAXBUF, NULL, 0, 1) !=
                0)
                error("could not get name info");
            break;
        }
    }
    freeifaddrs(ifaddr);
    return ip;
}

#ifndef assert
#define assert(COND)                                                                              \
    if (!(COND)) {                                                                                \
        printf("\n\033[0;31mAssertion failed: %s:%d\033[0m \"%s\"\n", __FILE__, __LINE__, #COND); \
        result = 1;                                                                               \
        goto EndOfTest;                                                                           \
    }
#endif

#define assert_eq(C1, C2) assert((C1) == (C2))
#define assert_str_eq(STR1, STR2) assert(strcmp(STR1, STR2) == 0)
#define assert_strn_eq(s1, s2, n) assert(strncmp(s1, s2, n) == 0)

struct testable
{
    char* name;
    int (*test_fn)(void);
};

// clang-format off
#define ADD_TEST(NAME) { #NAME, test_##NAME }

#define TEST(NAME, CLOSURE)                                                                       \
    int test_##NAME(void)                                                                         \
    {                                                                                             \
        int result = 0;                                                                           \
        CLOSURE;                                                                                  \
    EndOfTest:                                                                                    \
        return result;                                                                            \
    }

int RunAllTests(struct testable tests[])
{
    struct testable t;
    int i = 0, failure = 0, result;

    while ((t = tests[i++]).name != NULL) {
        result = t.test_fn();
        if (result != 0) {
            printf("%s failed!\n", t.name);
            failure = 1;
        }
        else {
            printf(".");
        }
    }

    if (failure)
        printf("\n\033[0;31mFAILURE\033[0m\n");
    else
        printf("\n\033[0;32mPASS\033[0m\n");
    return failure;
}

#define TEST_SUITE(...) static struct testable _main_tests[] = { __VA_ARGS__, { NULL, NULL } };

#define RUN_TEST_SUITE(...)                                                                       \
    TEST_SUITE(__VA_ARGS__);                                                                      \
    int main() { return RunAllTests(_main_tests); }

#define RunTests() RunAllTests(_main_tests)

TEST(parse_endpoint, ({
         char* urls[] = { "www.google.com:80", "golang.org:443", ":8080" };
         struct s_endpoint ep;
         ep.address = malloc(256);

         assert(parse_endpoint(urls[0], &ep) == 0);
         assert_strn_eq(ep.address, "www.google.com", 14);
         assert(ep.port == 80);
         bzero(ep.address, 256);

         assert(parse_endpoint(urls[1], &ep) == 0);
         assert_strn_eq(ep.address, "golang.org", 10);
         assert_eq(ep.port, 443);
         bzero(ep.address, 256);

         assert(parse_endpoint(urls[2], &ep) == 0);
         assert_str_eq(ep.address, "");
         assert(ep.port == 8080);

         assert_eq(-1, parse_endpoint("portlesshost", &ep));
         assert_eq(-1, parse_endpoint("colonbutnoport:", &ep));

         free(ep.address);
     }))

TEST(port_atoi, ({
         assert_eq(port_atoi("65535"), 65535);
         assert_eq(port_atoi("1"), 1);
         assert_eq(port_atoi("0"), 0);
         assert_eq(port_atoi("8080"), 8080);
         assert_eq(port_atoi("9876"), 9876);
         assert_eq(port_atoi("54321"), 54321);
         assert_eq(port_atoi("65536"), 0); // 65,536 is bigger than uint16_t
         assert_eq(port_atoi("0199"), 199);
         assert_eq(port_atoi("1 230"), 1230);
         assert_eq(port_atoi("8w"), 0);
         return 0;
     }))

TEST(grab_header_key, ({
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
     }))

#define FREE_SOCK                                                                                 \
    free(s.endpoint->address);                                                                    \
    free(s.endpoint);

TEST(Dial, ({
         socket_t s;
         assert_eq(Dial(&s, 23, "www.google.com:80"), ERR_BAD_PROTOCOL);

         assert_eq(Dial(&s, S_TCP, ""), ERR_BAD_ADDRESS);
         FREE_SOCK
         assert_eq(Dial(&s, S_TCP, "www.google.com:"), ERR_BAD_ADDRESS);
         FREE_SOCK
         assert_eq(Dial(&s, S_TCP, "www.google.com"), ERR_BAD_ADDRESS);
         FREE_SOCK

         bzero(s.endpoint, sizeof(struct s_endpoint));
         assert_eq(Dial(&s, S_TCP, "www.google.com:80"), 0);
         Close(&s);
         return 0;
     }))
// clang-format off
TEST(Listen, ({
    char* ip = find_host_ip();
    int i = strlen(ip);
    socket_t s;

    assert_eq(Listen(&s, S_TCP, ":8080"), 0);
    Close(&s);
    assert_eq(Listen(&s, S_TCP, ""), ERR_BAD_ADDRESS);
    FREE_SOCK
    assert_eq(Listen(&s, S_TCP, "257.0.0.1:8888"), ERR_BAD_ADDRESS);
    Close(&s);

    ip[i] = ':';
    ip[i + 1] = '8';
    ip[i + 2] = '0';
    ip[i + 3] = '9';
    ip[i + 4] = '1';
    ip[i + 5] = '\0';
    assert_eq(Listen(&s, S_TCP, ip), 0);
    assert_eq(s.endpoint->port, 8091);
    Close(&s);
    ip[i] = 0;
    assert_eq(Listen(&s, S_TCP, ip), ERR_BAD_ADDRESS);
    Close(&s);
    ip[i] = ':';
    ip[i + 1] = 0;
    assert_eq(Listen(&s, S_TCP, ip), ERR_BAD_ADDRESS);
    Close(&s);

    free(ip);
    return 0;
}))
#undef FREE_SOCK

void print_file(char* fname)
{
    FILE* fp = fopen(fname, "r");
    char c;

    while ((c = fgetc(fp)) != EOF)
        printf("%c", c);
    fclose(fp);
}

RUN_TEST_SUITE(ADD_TEST(parse_endpoint),
               ADD_TEST(port_atoi),
               ADD_TEST(grab_header_key),
               ADD_TEST(Listen),
               ADD_TEST(Dial));