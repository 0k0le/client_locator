/*
    Matthew Todd Geiger
    2019-04-05

    Basic file control library with automatic error control with irc_err.h
*/

#ifndef __IRC_FCNTL_H_
#define __IRC_FCNTL_H_

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "irc_mem.h"
#include "irc_err.h"
#include "irc_types.h"

int ec_open(const char *, int);
int ec_read(int, char *, int);
int ec_write(int, const char *, int);
int get_file_length(int);
int copy_to_memory(int, char *);

int copy_to_memory(int fd, char *buffer) {
    int len = get_file_length(fd);
    int read = ec_read(fd, buffer, len);

    return read;
}

int get_file_length(int fd) {
    int len = lseek(fd, 0, SEEK_END) + 1;

    if(len < 0) {
        error_exit("Failed to get file length");
    }

    lseek(fd, 0, SEEK_SET);

    return len;
}

int ec_write(int fd, const char *buf, int len) {
    int ret = 0;
    if((ret = write(fd, buf, len)) <= 0) {
        if(errno != 0)
            fprintf(stderr, "Failed to write to file\n");

        error_exit("Failed to write to file");
    }

    return ret;
}

int ec_read(int fd, char *buffer, int len) {
    int ret = 0;
    if((ret = read(fd, buffer, len)) <= 0) {
        if(errno == 0)
            return -1;
            
        //error_exit("Failed to read file contents");
    }

    lseek(fd, 0, SEEK_SET);

    return ret;
}

int ec_open(const char *file, int flags) {
    int fd = open(file, flags);
    if(fd < 0) {
        fprintf(stderr, "errno: %d", errno);
        if(errno != 0)
            error_exit("Failed to write to file");
    }

    return fd;
}

#endif