#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tcp.h"

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

int tcp_to_sockaddr(struct tcp_addr tcp_addr, struct sockaddr_in* sock_addr)
{
    CHECK_IF(sock_addr == NULL, return TCP_FAIL, "sock_addr is null");

    int chk = inet_pton(AF_INET, tcp_addr.ipv4, &sock_addr->sin_addr);
    CHECK_IF(chk != 1, return TCP_FAIL, "inet_pton failed");

    sock_addr->sin_family = AF_INET;
    sock_addr->sin_port   = htons(tcp_addr.port);
    return TCP_OK;
}

int tcp_to_tcpaddr(struct sockaddr_in sock_addr, struct tcp_addr* tcp_addr)
{
    CHECK_IF(tcp_addr == NULL, return TCP_FAIL, "tcp_addr is null");

    inet_ntop(AF_INET, &sock_addr.sin_addr, tcp_addr->ipv4, INET_ADDRSTRLEN);
    tcp_addr->port = ntohs(sock_addr.sin_port);
    return TCP_OK;
}

int tcp_server_init(struct tcp_server* server, char* local_ip, int local_port, int max_conn_num)
{
    CHECK_IF(server == NULL, return TCP_FAIL, "server is null");
    CHECK_IF(max_conn_num <= 0, return TCP_FAIL, "max_conn_num = %d invalid", max_conn_num);

    int chk;
    struct sockaddr_in me = {};
    me.sin_family = AF_INET;
    if (local_ip)
    {
        chk = inet_pton(AF_INET, local_ip, &me.sin_addr);
        CHECK_IF(chk != 1, return TCP_FAIL, "inet_pton failed");
    }
    else
    {
        me.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    me.sin_port = htons(local_port);

    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_IF(server->fd < 0, return TCP_FAIL, "sock failed");

    int on = 1;
    chk = setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    CHECK_IF(chk < 0, goto _ERROR, "setsockopt reuseaddr failed");

    chk = bind(server->fd, (struct sockaddr*)&me, sizeof(me));
    CHECK_IF(chk < 0, goto _ERROR, "bind failed");

    chk = listen(server->fd, max_conn_num);
    CHECK_IF(chk < 0, goto _ERROR, "listen failed");

    server->local_port = local_port;
    server->is_init    = 1;
    return TCP_OK;

_ERROR:
    if (server->fd > 0)
    {
        close(server->fd);
        server->fd = -1;
    }
    return TCP_FAIL;
}

int tcp_server_uninit(struct tcp_server* server)
{
    CHECK_IF(server == NULL, return TCP_FAIL, "server is null");
    CHECK_IF(server->is_init == 0, return TCP_FAIL, "server is not init yet");

    if (server->fd > 0)
    {
        close(server->fd);
        server->fd = -1;
    }
    server->is_init = 0;
    return TCP_OK;
}

int tcp_server_accept(struct tcp_server* server, struct tcp* tcp)
{
    CHECK_IF(server == NULL, return TCP_FAIL, "server is null");
    CHECK_IF(server->is_init == 0, return TCP_FAIL, "server is not init yet");
    CHECK_IF(server->fd < 0, return TCP_FAIL, "server fd = %d invalid", server->fd);
    CHECK_IF(tcp == NULL, return TCP_FAIL, "tcp is null");

    struct sockaddr_in remote = {};
    int addrlen = sizeof(remote);

    int fd = accept(server->fd, (struct sockaddr*)&remote, &addrlen);
    CHECK_IF(fd < 0, return TCP_FAIL, "accept failed");

    int chk = tcp_to_tcpaddr(remote, &tcp->remote);
    CHECK_IF(chk != TCP_OK, goto _ERROR, "tcp_to_tcpaddr failed");

    tcp->fd = fd;
    tcp->is_init = 1;

    return TCP_OK;

_ERROR:
    if (fd > 0)
    {
        close(fd);
    }
    return TCP_FAIL;
}

int tcp_client_init(struct tcp* tcp, char* remote_ip, int remote_port, int local_port)
{
    CHECK_IF(tcp == NULL, return TCP_FAIL, "tcp is null");
    CHECK_IF(remote_ip == NULL, return TCP_FAIL, "remote_ip is null");
    CHECK_IF(tcp->is_init != 0, return TCP_FAIL, "tcp has been init");

    struct sockaddr_in remote = {};
    remote.sin_family = AF_INET;

    int chk = inet_pton(AF_INET, remote_ip, &remote.sin_addr);
    CHECK_IF(chk != 1, return TCP_FAIL, "inet_pton failed");

    remote.sin_port = htons(remote_port);
    tcp->fd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_IF(tcp->fd < 0, return TCP_FAIL, "socket failed");

    if (local_port != TCP_PORT_ANY)
    {
        struct sockaddr_in me = {};
        me.sin_family = AF_INET;
        me.sin_addr.s_addr = htonl(INADDR_ANY);
        me.sin_port = htons(local_port);

        const int on = 1;
        chk = setsockopt(tcp->fd, SOL_SOCKET, SO_REUSEADDR, (void*)&on, sizeof(on));
        CHECK_IF(chk < 0, goto _ERROR, "setsockopt reuse failed");

        chk = bind(tcp->fd, (struct sockaddr*)&me, sizeof(me));
        CHECK_IF(chk < 0, goto _ERROR, "bind failed");
    }
    chk = connect(tcp->fd, (struct sockaddr*)&remote, sizeof(remote));
    CHECK_IF(chk < 0, goto _ERROR, "connect failed");

    chk = tcp_to_tcpaddr(remote, &tcp->remote);
    CHECK_IF(chk != TCP_OK, goto _ERROR, "tcp_to_tcpaddr failed");

    tcp->is_init = 1;
    return TCP_OK;

_ERROR:
    if (tcp->fd > 0)
    {
        close(tcp->fd);
        tcp->fd = -1;
    }
    return TCP_FAIL;
}

int tcp_client_uninit(struct tcp* tcp)
{
    CHECK_IF(tcp == NULL, return TCP_FAIL, "tcp is null");
    CHECK_IF(tcp->is_init == 0, return TCP_FAIL, "tcp is not init yet");

    if (tcp->fd > 0)
    {
        close(tcp->fd);
        tcp->fd = -1;
    }
    tcp->is_init = 0;
    return TCP_OK;
}

int tcp_recv(struct tcp* tcp, void* buffer, int buffer_size)
{
    CHECK_IF(tcp == NULL, return -1, "tcp is null");
    CHECK_IF(tcp->is_init == 0, return -1, "tcp is not init yet");
    CHECK_IF(tcp->fd <= 0, return -1, "tcp fd <= 0");
    CHECK_IF(buffer == NULL, return -1, "buffer is null");
    CHECK_IF(buffer_size <= 0, return -1, "buffer_size = %d invalid", buffer_size);
    return recv(tcp->fd, buffer, buffer_size, 0);
}

int tcp_send(struct tcp* tcp, void* data, int data_len)
{
    CHECK_IF(tcp == NULL, return -1, "tcp is null");
    CHECK_IF(tcp->is_init == 0, return -1, "tcp is not init yet");
    CHECK_IF(tcp->fd <= 0, return -1, "tcp fd <= 0");
    CHECK_IF(data == NULL, return -1, "data is null");
    CHECK_IF(data_len <= 0, return -1, "data_len = %d invalid", data_len);
    return send(tcp->fd, data, data_len, 0);
}
