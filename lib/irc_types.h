/*
    Matthew Todd Geiger
    2019-04-05

    Type definitions for ease of use
*/

#ifndef __IRC_TYPES_H_
#define __IRC_TYPES_H_

#include <sys/types.h>
#include <sys/stat.h>

#define _PROC_FOLDER "/proc/"
#define _PROC_STATUS "status"
#define _PROC_STATUS_BUFFER_SIZE 4096
#define _PROC_STATUS_NAME_OFFSET 0x6
#define _PROC_STATUS_ID_SIZE 0x4

#define _NEW_LINE_CHARACTER '\n'

#define bool u_int8_t

#define TRUE 1
#define FALSE 0

#define __STD_ARGS ...

char *first_half_buffer;
char *second_half_buffer;

int ml, rl, bm, br;

#endif