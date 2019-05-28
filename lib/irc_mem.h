/*
    Matthew Todd Geiger
    2019-04-05

    Call the startup routine to initialize pointer tracking for the heap
    When you call the cleanup routine all memory is freed automatically

    If you want to add memory use ec_malloc
    If you want to just free one piece of memory use ec_free
*/

#ifndef __IRC_MEM_H_
#define __IRC_MEM_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>

#include "irc_types.h"
#include "irc_err.h"

void irc_mem_startup();
void irc_mem_cleanup();
static void *ec_malloc_n(unsigned int);
void *ec_malloc(unsigned int);
void *ec_malloc_thread(unsigned int, pthread_mutex_t);
void ec_free(void *);
void ec_free_thread(void *, pthread_mutex_t);
int organize_list();
int write_list(void *);

static void **list = NULL;
static unsigned int mem_list_index = 0;

bool organizing = FALSE;
bool writing = FALSE;

// Allocate memory
void irc_mem_startup() {
    list = (void **)ec_malloc_n(sizeof(void *) * 320000);
}

// Cleanup memory automatically
void irc_mem_cleanup() {
    unsigned int ref = 0;

    if (memcmp(&list[0], &ref, sizeof(void *)) != 0) {
        for (int i = 0; i <= mem_list_index; i++) {
            free(list[i]);
        }
    }

    free(list);

    mem_list_index = 0;
}

// Error checking malloc
static void *ec_malloc_n(unsigned int size) {
    void *address = (void *)malloc(size);
    if(address == NULL) {
        error_exit("Failed to Allocate Memory to the Heap");
    }

    return address;
}

// Reorganize list
int organize_list() {
    if(organizing == TRUE)
        return -1;

    organizing = TRUE;
    
    void *ptr = 0;

    if(list == NULL)
        error_exit("List is not initialized");

    for(int i = 0; i <= mem_list_index; i++) {
        // Find NULL memory
        if(memcmp(&list[i], &ptr, sizeof(void *)) == 0) {
            for(int j = i + 1; j <= mem_list_index; j++) {
                // Copy data from address x to address x - 1
                memcpy(&list[j - 1], &list[j], sizeof(void *));
            }

            mem_list_index--;

            organizing = FALSE;
            return 0;
        }
    }

    error_exit("Failed to organize list");
}

// Free address from list and memory
void ec_free(void *address) {
    //printf("ec_free @ %p\n", address);
    //fflush(stdout);

    if(list == NULL)
        error_exit("List is not initialized");

    for(int i = 0; i <= mem_list_index; i++) {
        if(memcmp(&list[i], &address, sizeof(void *)) == 0) {
            free(address);
            memset(&list[i], 0, sizeof(void *));
            
            while(organize_list() != 0){}

            return;
        }
    }

    error_exit("Address does not exist");
}

void ec_free_thread(void *address, pthread_mutex_t thread_mutex) {
    pthread_mutex_lock(&thread_mutex);
    
    if(list == NULL)
        error_exit("List is not initialized");

    for(int i = 0; i <= mem_list_index; i++) {
        if(memcmp(&list[i], &address, sizeof(void *)) == 0) {
            free(address);
            memset(&list[i], 0, sizeof(void *));

            while(organize_list() != 0){}
            
            return;
        }
    }

    pthread_mutex_unlock(&thread_mutex);

    error_exit("Address does not exist");
}


// Error check and allocate memory
void *ec_malloc(unsigned int size) {
    // Allocate memory
    void *address = (void *)malloc(size);
    if(address == NULL) {
        error_exit("Failed to Allocate Memory to the Heap");
    }

    // Null out memory
    memset(address, 0, size);

    while(write_list(address) != 0) {}

    return address;
}

// Error check and allocate memory
void *ec_malloc_thread(unsigned int size, pthread_mutex_t thread_mutex) {
    // Allocate memory

    pthread_mutex_lock(&thread_mutex);

    void *address = (void *)malloc(size);
    if(address == NULL) {
        error_exit("Failed to Allocate Memory to the Heap");
    }

    // Null out memory
    memset(address, 0, size);

    while(write_list(address) != 0) {}

    pthread_mutex_unlock(&thread_mutex);

    return address;
}

int write_list(void *address) {
    if(writing == TRUE)
        return -1;

    writing = TRUE;

    // Update list
    memcpy(&list[mem_list_index], &address, sizeof(void *));
    mem_list_index++;

    writing = FALSE;

    return 0;
}

#endif