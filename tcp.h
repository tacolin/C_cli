#ifndef _TCP_H_
#define _TCP_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define TCP_PORT_ANY -1
#define TCP_OK (0)
#define TCP_FAIL (-1)

struct tcp_addr
{
    char ipv4[INET_ADDRSTRLEN];
    int  port;
};

struct tcp
{
    int fd;
    int local_port;
    int is_init;
    struct tcp_addr remote;
};

struct tcp_server
{
    int fd;
    int local_port;
    int is_init;
};

int tcp_server_init(struct tcp_server* server, char* local_ip, int local_port, int max_conn_num);
int tcp_server_uninit(struct tcp_server* server);
int tcp_server_accept(struct tcp_server* server, struct tcp* tcp);

int tcp_client_init(struct tcp* tcp, char* remote_ip, int remote_port, int local_port);
int tcp_client_uninit(struct tcp* tcp);

int tcp_recv(struct tcp* tcp, void* buffer, int buffer_size);
int tcp_send(struct tcp* tcp, void* data, int data_len);

int tcp_to_sockaddr(struct tcp_addr tcp_addr, struct sockaddr_in* sock_addr);
int tcp_to_tcpaddr(struct sockaddr_in sock_addr, struct tcp_addr* tcp_addr);

#endif //_TCP_H_
