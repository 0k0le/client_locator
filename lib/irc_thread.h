#ifndef __IRC_THREAD_H_
#define __IRC_THREAD_H_

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "irc_err.h"
#include "irc_types.h"
#include "irc_mem.h"

struct create_thread_data {
    char *data;
    pthread_t tid;
    pthread_attr_t attr;
};

typedef struct create_thread_data t_data, *pt_data;

static pt_data list_thread = NULL;
static unsigned int list_thread_index = 0;

void irc_thread_startup();
void irc_thread_cleanup();
void irc_thread_exit(void *);
void irc_create_thread(pthread_t *, void *, void *, int);
void irc_remove_thread(pthread_t *);
static void thread_organize_list();

static void thread_organize_list() {
    t_data temp = { 0 };
    memset(&temp, 0, sizeof(t_data));

    if(list_thread == NULL) {
        error_exit("Thread library not initialized");
    }

    for(int i = 0; i <= list_thread_index; i++) {
        // Find NULL memory
        if(memcmp(&list_thread[i], &temp, sizeof(t_data)) == 0) {
            for(int j = i + 1; j <= list_thread_index; j++) {
                // Copy data from address x to address x - 1
                memcpy(&list[j - 1], &list[j], sizeof(t_data));
            }

            return;
        }
    }

    error_exit("Failed to organize thread data");
}

void irc_thread_startup() {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    list_thread = (pt_data)ec_malloc(sizeof(t_data) * 4096);
}

void irc_thread_cleanup() {
    t_data temp = { 0 };
    memset(&temp, 0, sizeof(t_data));

    printf("Num: %d\n", list_thread_index);

    if(memcmp(&list_thread[0], &temp, sizeof(t_data)) != 0) {
        for(int i = 0; i < list_thread_index; i++) {
            pthread_cancel(list_thread[i].tid);
        }
    }
}

void irc_create_thread(pthread_t *tid, void *func, void *thread_data, int len) {
    if(list_thread == NULL) {
        error_exit("Thread library not initialized");
    }

    // Initialize struct
    pthread_attr_init(&list_thread[list_thread_index].attr);

    if(thread_data != NULL) {
        list_thread[list_thread_index].data = (void *)ec_malloc(len);
        memcpy(list_thread[list_thread_index].data, thread_data, len);
    } else {
        thread_data = NULL;
    }

    int ret = pthread_create(tid, &list_thread[list_thread_index].attr, func, list_thread[list_thread_index].data);
    if(ret != 0) {
        error_exit("Failed to create thread");
    }

    memcpy(&list_thread[list_thread_index].tid, tid, sizeof(pthread_t));

    list_thread_index++;
}

#ifdef __GNUC__
__attribute__ ((__noreturn__))
#endif
void irc_thread_exit(void *ret) {
    if(list_thread == NULL) {
        error_exit("Thread library no initialized");
    }

    pthread_t tid = pthread_self();

    for(int i = 0; i < list_thread_index; i++) {
        if(memcmp(&list_thread[i].tid, &tid, sizeof(pthread_t)) == 0) {
            ec_free(list_thread[i].data);
            memset(&list_thread[i], 0, sizeof(t_data));
            
            thread_organize_list();
            
            list_thread_index--;
            
            pthread_exit(ret);
        }
    }

    error_exit("Failed to organize thread cache");
}

void irc_remove_thread(pthread_t *tid) {
    if(list_thread == NULL) {
        error_exit("Thread library no initialized");
    }

    for(int i = 0; i < list_thread_index; i++) {
        if(memcmp(&list_thread[i].tid, tid, sizeof(pthread_t)) == 0) {
            pthread_cancel(*tid);

            ec_free(list_thread[i].data);
            memset(&list_thread[i], 0, sizeof(t_data));
            
            thread_organize_list();
            
            list_thread_index--;
            
            return;
        }
    }

    error_exit("Failed to organize thread cache");
}

#endif