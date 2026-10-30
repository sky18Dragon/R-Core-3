#pragma once
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#ifdef __cplusplus
extern "C" {
#endif
int dns_test_socket(int, int, int);
int dns_test_bind(int, const struct sockaddr *, socklen_t);
int dns_test_setsockopt(int, int, int, const void *, socklen_t);
ssize_t dns_test_recvfrom(int, void *, size_t, int, struct sockaddr *, socklen_t *);
ssize_t dns_test_sendto(int, const void *, size_t, int, const struct sockaddr *, socklen_t);
int dns_test_close(int);
#ifdef __cplusplus
}
#endif
#define socket dns_test_socket
#define bind dns_test_bind
#define setsockopt dns_test_setsockopt
#define recvfrom dns_test_recvfrom
#define sendto dns_test_sendto
#define close dns_test_close
