//
// Programmer:      Matthew Todd Geiger
// Creation Date:   Wed May     8 11:15:15 PST 2019
//
// Filename:        irc_curl.h
// Syntax:          C; CURL

/*
 *      Client Generation Tool For Freelancers API
 */

#ifndef __IRC_CURL_H_
#define __IRC_CURL_H_

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

// lib includes
#include "irc_err.h"
#include "irc_mem.h"
#include "irc_types.h"

// Curl includes
#include <curl/curl.h>

// Define curl function options
#define CURL_POST_OPT 0b10000000
#define CURL_GET_OPT 0b01000000
#define CURL_FILE_OPT 0b00100000
#define CURL_FOLLOW_OPT 0b00010000
#define CURL_VERB_OPT 0b00001000
#define CURL_COOKIE_OPT 0b00000100
#define CURL_SET_COOKIE_OPT 0b00000010

void curl_slist_free_all_c(struct curl_slist *list, void (*freeheader)(void *))
{
    struct curl_slist *next;
    struct curl_slist *item;

    if (!list)
        return;

    item = list;
    do
    {
        next = item->next;

        if (item->data && freeheader)
        {
            freeheader(item->data);
        }
        free(item);
        item = next;
    } while (next);
}

static void
print_cookies(CURL *curl)
{
    CURLcode res;
    struct curl_slist *cookies;
    struct curl_slist *nc;
    int i;

    printf("\n\nCookies, curl knows:\n\n");
    res = curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookies);
    if (res != CURLE_OK)
    {
        fprintf(stderr, "Curl curl_easy_getinfo failed: %s\n",
                curl_easy_strerror(res));
        exit(1);
    }
    nc = cookies;
    i = 1;
    while (nc)
    {
        printf("[%d]: %s\n", i, nc->data);
        nc = nc->next;
        i++;
    }
    if (i == 1)
    {
        printf("(none)\n");
    }
    curl_slist_free_all(cookies);
}

int create_http_header(CURL *curl, char *buffer, struct curl_slist *list)
{
    printf("Creating http header\n");

    list = NULL;
    char head[4096];
    char *ptr = buffer;
    while ((ptr = strstr(ptr, "-H")) != NULL)
    {
        memset(head, 0, 4096);

        ptr = ptr + 4;

        int i = 0;
        for (i = 0; ptr[i] != '\''; i++)
        {
            memset(head + i, ptr[i], 1);
        }
        memset(head + i + 1, 0, 1);

        list = curl_slist_append(NULL, head);
        printf("%s\n", head);
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

    printf("\n\n");

    return 0;
}

char *organize_post_data(char *post_data) {
    char *ptr = strstr(post_data, "--data");
    if(ptr == NULL) {
        perror("Failed to locate post info");
        return NULL;
    }

    ptr = ptr + 8;

    int j = 0;
    while(*(ptr + 1) != 0 && *ptr != '\'') {
        memset(post_data + j, *ptr, 1);
        //printf("%c", post_data[j]);
        ptr++;
        j++;
    }

    memset(post_data + j, 0, 1);

    printf("%s\n", post_data);

    return post_data;
}

int curl_web(const char *link, u_int8_t opt, const char *curl_file, const char *scrape_file, const char *proxy, const char *proxy_auth)
{
    /*Section 1: Buffer file contents*/
    // Open file
    int fd = open(curl_file, O_RDWR);
    if(fd < 0) {
        fprintf(stderr, "Failed to open file: %s", curl_file);
        return -1;
    }

    // Get file length
    int fd_length = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    if(fd_length <= 0) {
        perror("Failed to get file length");
        return -1;
    }

    // Create file local buffer on heap
    char *fd_buffer = (char *)malloc(sizeof(char) * (fd_length + 1));
    if(fd_buffer == NULL) {
        perror("Failed to allocate memory on heap");
        return -1;
    }

    // Read file contents to buffer
    if(read(fd, fd_buffer, fd_length) == -1) {
        perror("Failed to read file contents to buffer");
        return -1;
    }

    // Close file descriptor
    close(fd);

    /*Section 2: Create CURL*/
    // Initialize options
    u_int8_t upost = 0, uget = 0, ufile = 0, ufollow = 0, ucookie = 0, ucookie_set = 0;
    if(opt & CURL_FILE_OPT)
        ufile = 1;
    if(opt & CURL_GET_OPT)
        uget = 1;
    if(opt & CURL_POST_OPT)
        upost = 1;
    if(opt & CURL_FOLLOW_OPT)
        ufollow = 1;
    if(opt & CURL_COOKIE_OPT)
        ucookie = 1;
    if(opt & CURL_SET_COOKIE_OPT)
        ucookie_set = 1;

    if(opt & CURL_VERB_OPT) {
        printf("VERBOSE: CURL_FILE_OPT: %d | CURL_GET_OPT: %d | CURL_POST_OPT: %d | CURL_FOLLOW_OPT: %d\n",
            ufile, uget, upost, ufollow);
    }

    if(upost == uget) {
        fprintf(stderr, "curl_post(): INVALID OPTIONS\t<upost and uget cannot be the same value>\n");

        free(fd_buffer);
        return -1;
    }

    // Initialize lib CURL
    CURL *curl;
    CURLcode res;

    struct curl_slist *list = NULL;

    curl = curl_easy_init();


    if(!curl) {
        perror("Failed to get curl handle");
        return -1;
    }

    if(ucookie) {
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "cookiejar.dat");
    }

    if(ucookie_set) {
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookiejar.dat");
    }

    if(opt & CURL_VERB_OPT) {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }

    if(create_http_header(curl, fd_buffer, list) != 0) {
        perror("Failed to create http header");

        free(fd_buffer);
        return -1;
    }

    if(uget)
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    else if(upost) {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 0L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, organize_post_data(fd_buffer));
    } else {
        fprintf(stderr, "curl_post(): FATAL: Invalid settings detected\n");

        free(fd_buffer);
        return -1;
    }

    if(ufollow)
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl, CURLOPT_URL, link);

    FILE *file;
    if (ufile)
    {
        remove(scrape_file);
        file = fopen(scrape_file, "w");
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    }

    if(proxy != NULL) {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxy);
        curl_easy_setopt(curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
        if(proxy_auth != NULL) {
            curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, proxy_auth);
        }
    }

#ifdef SKIP_PEER_VERIFICATION
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

#ifdef SKIP_HOSTNAME_VERIFICATION
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        return -1;
    }

    if(ucookie)
        print_cookies(curl);

    // Cleanup
    curl_slist_free_all_c(list, free);
    curl_easy_cleanup(curl);

    free(fd_buffer);

    if(ufile)
        fclose(file);

    return 0;
}

#endif