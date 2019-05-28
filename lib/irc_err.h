/*
    Matthew Todd Geiger
    2019-04-05

*/

#ifndef __IRC_ERR_H_
#define __IRC_ERR_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include "irc_types.h"

// Handles exit process
#ifdef __GNUC__
__attribute__ ((__noreturn__))
#endif
static void
terminate(bool bExit) {
    char *s = getenv("EF_DUMPCORE");

    /* If EF_DUMPCORE is defined and non-null, dump core w/ abort() */

    if(s != NULL && *s != 0)
        abort();
    else if(bExit)
        exit(EXIT_FAILURE);
    else
        _exit(EXIT_FAILURE);
    
}

// Force fail termination
#ifdef __GNUC__
__attribute__ ((__noreturn__))
#endif
void
fatal() {
    terminate(FALSE);
}

// General error exit
#ifdef __GNUC__
__attribute__ ((__noreturn__))
#endif
void
error_exit(const char *format, __STD_ARGS) {
    va_list arg_list;

    fflush(stdout);

    fprintf(stderr, "ERROR: ");

    va_start(arg_list, format);
    vfprintf(stderr, format, arg_list);
    va_end(arg_list);

    fprintf(stderr, " | %s\n", strerror(errno));

    fflush(stderr);
    terminate(TRUE);
}

// Same as error_exit but can control the terminate function
#ifdef __GNUC__
__attribute__ ((__noreturn__))
#endif
void
error_exit_c(const char *format, bool bool_exit, __STD_ARGS) {
    va_list arg_list;

    fflush(stdout);

    fprintf(stderr, "ERROR: ");

    va_start(arg_list, bool_exit);
    vfprintf(stderr, format, arg_list);
    va_end(arg_list);

    fprintf(stderr, " | %s\n", strerror(errno));

    fflush(stderr);
    terminate(bool_exit);
}

#endif
