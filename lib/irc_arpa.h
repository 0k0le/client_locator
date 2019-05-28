/*
    Matthew Todd Geiger
    2019-04-05

    Basic networking library with automatic error control
*/


#ifndef __IRC_ARPA_H_
#define __IRC_ARPA_H_

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>

#include "irc_err.h"
#include "irc_types.h"
#include "irc_mem.h"
#include "irc_fcntl.h"

int ec_send(int, void *, size_t, int);
int ec_recv(int, void *, size_t, int);
int ec_socket(int, int, int);
int ec_accept(int, struct sockaddr *, socklen_t);
void ec_bind(int, const struct sockaddr *, socklen_t);
void ec_connect(int, const struct sockaddr *, socklen_t);
void ec_listen(int, int);
void ec_setsockopt(int, int, int);

void create_sockaddrin(struct sockaddr_in *, int, int, const char *);

void create_sockaddrin(struct sockaddr_in *sin, int port, int family, const char *address) {
    memset(sin, 0, sizeof(struct sockaddr_in));
    
    if(family > 0)
        sin->sin_family = family;

    if(port > 0)
        sin->sin_port = htons(port);

    if (address != NULL) {
        if (inet_pton(family, address, &sin->sin_addr) < 0) {
            error_exit("Invalid IP Address");
        }
    } else {
        sin->sin_addr.s_addr = INADDR_ANY;
    }
}

void ec_setsockopt(int fd, int level, int optname) {
    if(setsockopt(fd, level, optname, &(int){ 1 }, sizeof(int)) < 0) {
        error_exit("Failed to socket option");
    }
}

int ec_send(int fd, void *buf, size_t len, int flags) {
    int ret = 0;
    if((ret = send(fd, buf, len, flags)) <= 0) {
        error_exit("Failed to send data");
    }

    return ret;
}

int ec_recv(int fd, void *buf, size_t len, int flags) {
    int ret = 0;
    if((ret = recv(fd, buf, len, flags)) <= 0) {
        error_exit("Failed to recieve data");
    }

    return ret;
}

int ec_socket(int family, int type, int protocol){
    int sockfd = 0;
    if((sockfd = socket(family, type, protocol)) < 0) {
        error_exit("Failed to create socket");
    }

    return sockfd;
}

int ec_accept(int fd, struct sockaddr *sock, socklen_t len){
    int sockfd = 0;
    if((sockfd = accept(fd, sock, &len)) < 0) {
        error_exit("Failed to create accept connection");
    }

    return sockfd;
}

void ec_bind(int fd, const struct sockaddr *sock, socklen_t len) {
    if(bind(fd, sock, len) < 0) {
        error_exit("Failed to bind");
    }
}

void ec_connect(int fd, const struct sockaddr *sock, socklen_t len) {
    if(connect(fd, sock, len) < 0) {
        close(fd);
        error_exit("Failed to connect");
    }
}

void ec_listen(int fd, int amount) {
    if(listen(fd, amount) < 0) {
        error_exit("Failed to setup listener");
    }
}

#endif
