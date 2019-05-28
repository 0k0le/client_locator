#ifndef __IRC_CL_
#define __IRC_CL_

//
// Programmer:      Matthew Todd Geiger
// Creation Date:   Wed May     8 11:06:30 PST 2019
//
// Filename:        irc_cl.h
// Syntax:          C; CURL
// Make:            gcc main.c -Wall -lcrypt -lcurl -lpthread -o main

/*
 *      Client Generation Tool For Freelancers
 */


// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>

// lib includes
#include "irc_err.h"
#include "irc_mem.h"
#include "irc_fcntl.h"
#include "irc_types.h"
#include "irc_curl.h"

// Curl includes
#include <curl/curl.h>

// Defines
#define LINK_COUNT 8192

// Create variable to track if running
bool running = FALSE;

// Compile Options
//#define __THREAD_DEBUG_

// Launch functions
int launch_gig_scrapers(const char *, char **, char **, int, int);

// Library init/clean functions
void init_mem(void);
void clean_mem(void);

// Parsing functions
int parse_cl_main(const char *, const char *, char **);
int create_curl_dat(const char *, char *, char *);
int gather_gig_links(const char *, char **, char**, char**, int);
int remove_link_copies(const char *, char **, char **, int);

// HTML generation functions
int create_html_page(const char *, const char *, const char *, const char *, int);
int ec_write_table(const char *, int, bool);

// gig functions
void *find_computer_gigs(void *);

// MLP functions
char **mlp_malloc(int, int);
void mlp_free(char **, int);

// View functions
int view_refined(const char *);

// Public link strings
const char cl_link_main[] = "https://www.craigslist.org/about/sites";

// Create curl MUTEX
pthread_mutex_t curl_mutex;

// Thread data structure
typedef struct thread_curl_data {
    char *link;
    char link_temp[256];
    char prefix[4096];
    char suffix[4096];
} TCD, *pTCD;

int curl_web_thread(const char *link, u_int8_t opt, const char *curl_file, const char *scrape_file, const char *proxy, const char *proxy_auth, pthread_mutex_t mutex)
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

    pthread_mutex_lock(&mutex);

    curl = curl_easy_init();

    pthread_mutex_unlock(&mutex);


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

    pthread_mutex_lock(&mutex);

    res = curl_easy_perform(curl);

    pthread_mutex_unlock(&mutex);

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

void launch_data_miner(const int argc, const char *argv[]) {
    printf("-- Craigslist Console Scraper --\n\n");

    if(getuid() != 0)
        error_exit("Please run as root\n");

    nice(-20);

    // Init memory manager
    init_mem();

    // Create vars
    int line_count = 0;
    int gig_link_count = 0;
    int thread_count = 0;

    // Loop through arguments
    for(int i = 0; i < argc; i++) {
        if(strstr(argv[i], "--thread-count") != NULL && argv[i + 1][0] != 0) {
            thread_count = atoi(argv[i + 1]);
        }

        if(strstr(argv[i], "--help") != NULL) {
            printf("--- HELP ---\n");
            printf("--thread-count <NUM OF THREADS> (Define number of threads to use)\n");
            printf("--view-ref (View refined list)\n");
            printf("--help (Views Help)\n\n");

            exit(EXIT_SUCCESS);
        }

        if(strstr(argv[i], "--view-ref") != NULL) {
            printf("\nLink Count: %u\n", view_refined("../dat/gigs/refined_links.dat"));

            exit(EXIT_SUCCESS);
        }
    }

    // Default value
    if(thread_count == 0)
        thread_count = 4;

    printf("Initializing program...\n");
    printf("Threads: %u\n", thread_count);
#ifdef __THREAD_DEBUG_
    printf("WARNING: THREAD DEBUG ENABLED!\n");
#else
    printf("Thread Debug: DISABLED\n\n");
#endif

    printf("\n");

    remove("cookiejar.dat");

    // Scrape main craigslist site to gather all availiable links
    if (curl_web(cl_link_main,
                    (CURL_GET_OPT | CURL_VERB_OPT | CURL_FILE_OPT | CURL_FOLLOW_OPT | CURL_SET_COOKIE_OPT | CURL_COOKIE_OPT),
                    "curl/cl_main.dat", "../html/scrapes/cl_main.html", NULL, NULL) != 0)
    {
        error_exit("Failed to scrape main craigslist link");
    }

    // Allocate mlp for links
    char **link_list = mlp_malloc(LINK_COUNT, 4096);
    char **file_list = mlp_malloc(LINK_COUNT, 4096);

    // Parse for site links
    if((line_count = parse_cl_main("../html/scrapes/cl_main.html", "../dat/links.dat", link_list)) <= 0) {
        error_exit("Failed to parse html file");
    }

    // Initialize mutex
    if(pthread_mutex_init(&curl_mutex, NULL) != 0) {
        error_exit("Failed to initialize thread mutex");
    }

    // Launch scraper threads
    if(launch_gig_scrapers("curl/cl_sites.dat", link_list, file_list, line_count, thread_count) != 0) {
        error_exit("Failed to scrape gig links");
    }

    // Destory mutex
    pthread_mutex_destroy(&curl_mutex);

    mlp_free(link_list, LINK_COUNT);
    char **gig_list = mlp_malloc(LINK_COUNT, 4096);
    char **date_list = mlp_malloc(LINK_COUNT, 4096);

    fflush(stdout);

    for(int i = 0; i < line_count; i++) {
        printf("FILE: %s\n", file_list[i]);
        fflush(stdout);
#ifdef __THREAD_DEBUG_
        break;
#endif
    }

    if((gig_link_count = gather_gig_links("../dat/gigs/unrefined_links.dat", file_list, gig_list, date_list, line_count)) <= 0) {
        fprintf(stderr, "FATAL ERROR: gather_gig_links()\n");
        fflush(stdout);
        fflush(stderr);
        getchar();
        
        error_exit("Failed to gather links from html file\n");
    }

    printf("\ngig_link_count: %u\n", gig_link_count);

    if((gig_link_count = remove_link_copies("../dat/gigs/refined_links.dat", gig_list, date_list, gig_link_count)) <= 0) {
        fprintf(stderr, "FATAL ERROR: remove_link_copies()\n");
        fflush(stdout);
        fflush(stderr);
        getchar();

        error_exit("Failed to remove link copies");
    }

    printf("Final Gig Amount: %u\n", gig_link_count);
    fflush(stdout);

    // Free list
    mlp_free(file_list, LINK_COUNT);
    mlp_free(gig_list, LINK_COUNT);
    mlp_free(date_list, LINK_COUNT);

    create_html_page("../html/page/part_1.html", "../html/page/part_2.html",
        "web_server/views/main.ejs", "../dat/gigs/refined_links.dat", gig_link_count);

    fflush(stdout);
    fflush(stderr);

    // Clean up memory
    clean_mem();
    exit(EXIT_SUCCESS);
}

int ec_write_table(const char *buffer, int fd, bool link) {
    ec_write(fd, "<td>", 4);

    if(link) {
        if(strstr(buffer, "commis") != NULL) {
            ec_write(fd, "<a style=\"color: brown\">", strlen("<a style=\"color: brown\">"));
        } else if(strstr(buffer, "scrape") != NULL) {
            ec_write(fd, "<a style=\"color: red\">", strlen("<a style=\"color: red\">"));
        } else if(strstr(buffer, "contract") != NULL) {
            ec_write(fd, "<a style=\"color: green\">", strlen("<a style=\"color: green\">"));
        } else if(strstr(buffer, "built") != NULL || strstr(buffer, "build") != NULL) {
            ec_write(fd, "<a style=\"color: purple\">", strlen("<a style=\"color: purple\">"));
        } else if(strstr(buffer, "design") != NULL) {
            ec_write(fd, "<a style=\"color: blue\">", strlen("<a style=\"color: blue\">"));
        } else {
            ec_write(fd, "<a>", 3);
        }
    }
    
    ec_write(fd, buffer, strlen(buffer));
    
    if(link) ec_write(fd, "</a>", 4);

    ec_write(fd, "</td>", 5);
    return 0;
}

// Create final HTML document
int create_html_page(const char *part_1, const char *part_2, const char *output_file, const char *refined_file, int line_count) {
    printf("Allocating file buffers...\n");
    fflush(stdout);

    int part_1_fd = ec_open(part_1, O_RDONLY);
    char *part_1_buffer = (char *)ec_malloc(sizeof(char) * get_file_length(part_1_fd) + 32);
    copy_to_memory(part_1_fd, part_1_buffer);
    close(part_1_fd);

    int part_2_fd = ec_open(part_2, O_RDONLY);
    char *part_2_buffer = (char *)ec_malloc(sizeof(char) * get_file_length(part_2_fd) + 32);
    copy_to_memory(part_2_fd, part_2_buffer);
    close(part_2_fd);

    int final_file = ec_open(refined_file, O_RDONLY);
    char *final_file_buffer = (char *)ec_malloc(sizeof(char) * get_file_length(final_file) + 32);
    copy_to_memory(final_file, final_file_buffer);
    close(final_file);

    char **refined_link_list = mlp_malloc(LINK_COUNT, 4096);
    char **refined_date_list = mlp_malloc(LINK_COUNT, 4096);

    printf("Starting html page creation...\n");
    fflush(stdout);

    char *ptr = final_file_buffer;
    for(int i = 0; i < line_count; i++) {
        for(int j = 0; *ptr != ' ' && j < 4096; j++) {
            memset(refined_date_list[i] + j, *ptr, 1);
            ptr++;
        }
        ptr++;

        for(int j = 0; *ptr != '\n' && j < 4096; j++) {
            memset(refined_link_list[i] + j, *ptr, 1);
            ptr++;
        }
        ptr++;

        printf("Buffering: %s\n", refined_date_list[i]);
        printf("Buffering: %s\n", refined_link_list[i]);
        fflush(stdout);
    }

    printf("Starting final HTML stage...\n");
    fflush(stdout);

    remove(output_file);
    int output_fd = ec_open(output_file, O_WRONLY | O_CREAT);

    ec_write(output_fd, part_1_buffer, strlen(part_1_buffer));

    for(int i = 0; i < line_count; i++) {
        ec_write(output_fd, "<tr>", strlen("<tr>"));
        ec_write_table(refined_date_list[i], output_fd, FALSE);
        ec_write_table(refined_link_list[i], output_fd, TRUE);
        ec_write(output_fd, "</tr>", strlen("</tr>"));
    }

    ec_write(output_fd, part_2_buffer, strlen(part_2_buffer));

    printf("HTML page creation complete...\n");
    fflush(stdout);

    ec_free(part_1_buffer);
    ec_free(part_2_buffer);
    ec_free(final_file_buffer);

    mlp_free(refined_link_list, LINK_COUNT);
    mlp_free(refined_date_list, LINK_COUNT);

    close(output_fd);

    return 0;
}

// View refined link list
int view_refined(const char *file_name) {
    // Check for proper file name
    if(strstr(file_name, "dat") == NULL) {
        fprintf(stderr, "Improper file name\n");
        return 0;
    }

    int link_count = 0;

    // Open file
    int fd = ec_open(file_name, O_RDONLY);
    
    // Create file buffer
    char *fd_buffer = (char *)ec_malloc(sizeof(char) * get_file_length(fd) + 32);
    
    // Copy file to memory
    copy_to_memory(fd, fd_buffer);
    
    // Close file
    close(fd);

    // Print file output
    printf("%s\n", fd_buffer);

    // Count link amount
    char *ptr = fd_buffer;
    while((ptr = strstr(ptr, "\n")) != NULL) {
        ptr++;
        link_count++;
    }

    // Clean up
    free(fd_buffer);

    return link_count;
}

int remove_link_copies(const char *output_file, char **gig_list, char **date_list, int link_count) {
    if((strstr(output_file, "dat")) == NULL) {
        fprintf(stderr, "Invalid output file name\n");
        return -1;
    }

    // Count links applied to temp list
    int gig_counter = 0;

    // Create temp buffers
    char **temp_list = mlp_malloc(LINK_COUNT, 4096);
    char *temp_link = (char *)ec_malloc(sizeof(char) * 4096);

    // Open output file
    remove(output_file);
    int output_fd = ec_open(output_file, O_WRONLY | O_CREAT);

    // Main loop
    for(int i = 0; i < link_count; i++) {
        char *ptr = gig_list[i];

        // Cut link into basic string
        if((ptr = strstr(ptr, "/d/")) != NULL) {
            ptr = ptr + strlen("/d/");

            for(int j = 0; j < 4096 && *ptr != '/'; j++) {
                memset(temp_link + j, *ptr, 1);
                ptr++;
            }
        }

        bool found = FALSE;

        // Attempt to locate basic string in temp 
        for(int j = 0; j < link_count; j++) {
            if(strstr(temp_list[j], temp_link) != NULL) {
                found = TRUE;
                break;
            }
        }

        if(found == FALSE) {
            memcpy(temp_list[gig_counter], gig_list[i], strlen(gig_list[i]));
            printf("Final: %s %s\n", date_list[i], temp_list[gig_counter]);
            fflush(stdout);

            ec_write(output_fd, date_list[i], strlen(date_list[i]));
            ec_write(output_fd, " ", 1);

            ec_write(output_fd, temp_list[gig_counter], strlen(temp_list[gig_counter]));
            ec_write(output_fd, "\n", 1);
            gig_counter++;
        }

        memset(temp_link, 0, 4096);
    }

    // Clean up
    close(output_fd);
    ec_free(temp_link);
    mlp_free(temp_list, LINK_COUNT);

    printf("Remove links exiting...\n");
    fflush(stdout);

    return gig_counter;
}

int gather_gig_links(const char *output_file, char **file_list, char **gig_list, char **date_list, int file_count) {
    // Assign constants
    const char main_index[] = "data-pid=\"";
    const char href_index[] = "href=\"";
    const char date_index[] = "datetime=\"";

    // Define link counter
    int link_count = 0;

    // Check for valid output name
    if(strstr(output_file, "dat") == NULL) {
        fprintf(stderr, "Invalid output file name\n");
        return -1;
    }

    // Open output file
    remove(output_file);
    int output_fd = ec_open(output_file, O_RDWR | O_CREAT);

    // Create link buffer
    char *link = (char *)ec_malloc(sizeof(char) * 4096);
    char *date = (char *)ec_malloc(sizeof(char) * 4096);

    // Loop through all files
    for(int i = 0; i < file_count; i++) {
        // Copy html file to buffer
        int html_fd = ec_open(file_list[i], O_RDONLY);
        char *html_buffer = (char *)ec_malloc(sizeof(char) * get_file_length(html_fd) + 32);
        copy_to_memory(html_fd, html_buffer);
        close(html_fd);

        // Assign memory tracker
        char *ptr = html_buffer;

        // Main html loop
        while((ptr = strstr(ptr, main_index)) != NULL) {
            ptr = ptr + strlen(main_index);

            // Check for valid data-pid
            if(*ptr != '{' && *ptr != '#') {
                // Track to date
                if((ptr = strstr(ptr, date_index)) == NULL)
                    break;

                ptr = ptr + strlen(date_index);

                if(*ptr == '2' && *(ptr + 1) == '0') {
                    memset(date, 0, 4096);

                    for(int i = 0; i < 4095 && *ptr != ' ' && *ptr != 0; i++) {
                        memset(date + i, *ptr, 1);
                        ptr++;
                    }

                    // Write to output file
                    ec_write(output_fd, date, strlen(date));
                    ec_write(output_fd, " ", 1);

                    memcpy(date_list[link_count], date, strlen(date) + 1);

                    printf("GIG: %s ", date_list[link_count]);
                    fflush(stdout);

                    // Track to href
                    if((ptr = strstr(ptr, href_index)) == NULL)
                        break;

                    ptr = ptr + strlen(href_index);

                    // Confirm http(s) link
                    if(*ptr == 'h' || *ptr == 'H') {
                        // Null buffer
                        memset(link, 0, 4096);
                    
                        // Copy link
                        for(int i = 0; i < 4095 && *ptr != '\"' && *ptr != 0; i++) {
                            memset(link + i, *ptr, 1);
                            ptr++;
                        }

                        // Write to output file
                        ec_write(output_fd, link, strlen(link));
                        ec_write(output_fd, "\n", 1);

                        memcpy(gig_list[link_count], link, strlen(link) + 1);

                        printf("%s --> #%d\n", gig_list[link_count], link_count);
                        fflush(stdout);

                        link_count++;
                    }
                }
            }
        }

       ec_free(html_buffer);
    }

    ec_free(link);
    ec_free(date);
    close(output_fd);

    return link_count;
}

// Function to run main gig scrapers
int launch_gig_scrapers(const char *dat_file, char **link_list, char **file_list, int line_count, int thread_count) {
    if(thread_count <= 0) {
        fprintf(stderr, "Invalid thread count\n");
        return -1;
    }

    // Allocate memory for dat file prefix and suffix
    char *suf = (char *)ec_malloc(sizeof(char) * 4096);
    char *pre = (char *)ec_malloc(sizeof(char) * 4096);

    // Create prefix and suffix
    create_curl_dat(dat_file, pre, suf);

    // Create thread id and attributes
    pthread_t tids[line_count];
    pthread_attr_t attr;

    pthread_attr_init(&attr);

    // Create thread data structures
    TCD thread_data[line_count];

    printf("line_count: %d\n", line_count);

    // Create threads in groups of 128
    int closed = 0;
    for(int i = 0; i < line_count; i++) {
        // Copy data to thread structure
        memcpy(thread_data[i].prefix, pre, strlen(pre) + 1);
        memcpy(thread_data[i].suffix, suf, strlen(suf) + 1);
        
        // Splice link and copy
        char *ptr_temp = strstr(link_list[i], "/") + 2;

        int j = 0;
        for(; j < 4096 && *ptr_temp != '/'; j++) {
            memset(thread_data[i].link_temp + j, *ptr_temp, 1);
            ptr_temp++;
        }
        memset(thread_data[i].link_temp + j, 0, 1);

        // Complete link
        strcat(link_list[i], "search/cpg?query=web&is_paid=all&search_distance=100");
        thread_data[i].link = link_list[i];

        sprintf(file_list[i], "../html/scrapes/%s.html", thread_data[i].link_temp);

        // Launch thread
        if(pthread_create(&tids[i], &attr, find_computer_gigs, &thread_data[i]) < 0) {
            fprintf(stderr, "Failed to launch thread: %d\n", i);
            return -1;
        }
#ifdef __THREAD_DEBUG_
        break;
#endif

        // Only launch 128 threads at a time
        if(i % thread_count == 0) {
            int ref = i - thread_count;

            for(int h = ref; h < i; h++) {
                pthread_join(tids[i], NULL);
                closed++;
            }
        }
    }

    // Close remaining threads
    for(int i = closed; i < line_count; i++) {
        pthread_join(tids[i], NULL);
    }

    // Clean up memory
    ec_free(suf);
    ec_free(pre);

    fflush(stdout);

    return 0;
}

// Thread function for computer gig curl
void *find_computer_gigs(void *args) {
    printf("Thread launched\n");
    fflush(stdout);

    // Define link
    pTCD data = (pTCD)args;

    char dat_buffer[4096];
    char html_file[4096];
    char dat_file[4096];

    //data->link[strlen(data->link) - 1] = 0;

    sprintf(html_file, "../html/scrapes/%s.html", data->link_temp);
    sprintf(dat_buffer, "%s%s%s", data->prefix, data->link, data->suffix);
    sprintf(dat_file, "curl/%s.dat", data->link_temp);

    remove(html_file);
    remove(dat_file);

    printf("%s\n", dat_file);

    int dat_fd = ec_open(dat_file, O_RDWR | O_CREAT);
    ec_write(dat_fd, dat_buffer, strlen(dat_buffer));
    close(dat_fd);

    remove(html_file);
    int temp_fd = ec_open(html_file, O_RDWR | O_CREAT);
    close(temp_fd);

    // Lock mutex
    //pthread_mutex_lock(&curl_mutex);

    // Scrape main craigslist site to gather all availiable links
    while (curl_web_thread(data->link,
                    (CURL_GET_OPT | CURL_VERB_OPT | CURL_FILE_OPT | CURL_FOLLOW_OPT | CURL_SET_COOKIE_OPT | CURL_COOKIE_OPT),
                    dat_file, html_file, NULL, NULL, curl_mutex) != 0) {
        printf("Scanning %s\n", data->link);
    }

    // Unlock mutex
    //pthread_mutex_unlock(&curl_mutex);

    fflush(stdout);
    fflush(stderr);

    pthread_exit(0);
}

// Create new dat file
int create_curl_dat(const char *dat_file, char *prefix, char *suffix) {
    // Check for valid dat file
    if(strstr(dat_file, "dat") == NULL) {
        fprintf(stderr, "Invalid dat file");
        return -1;
    }

    // Copy dat file to memory
    int fd = ec_open(dat_file, O_RDONLY);
    char *file_buffer = (char *)ec_malloc(sizeof(char) * get_file_length(fd) + 32);
    copy_to_memory(fd, file_buffer);
    close(fd);

    char *ptr = file_buffer;

    // Create prefix
    int i = 0;
    for(; i < 4096 && *ptr != '\''; i++) {
        memset(prefix + i, *ptr, 1);
        ptr++;
    }
    memset(prefix + i, '\'', 1);
    ptr++;

    if((ptr = strstr(ptr, "\'")) == NULL) {
        fprintf(stderr, "Failed to create suffix\n");

        ec_free(prefix);
        ec_free(suffix);
        ec_free(file_buffer);

        return -1;
    }

    // Create suffix
    i = 0;
    for(; i < 4096 && *ptr != 0; i++) {
        memset(suffix + i, *ptr, 1);
        ptr++;
    } 

    // Test output
    printf("%s%s\n", prefix, suffix);

    ec_free(file_buffer);

    return 0;
}

// Allocate multi level ptr
char **mlp_malloc(int string_count, int string_size) {
    char **temp = (char **)ec_malloc(sizeof(char *) * string_count);
    for(int i = 0; i <= string_count; i++)
        temp[i] = (char *)ec_malloc(sizeof(char) * string_size);

    return temp;
}

// Free multi level ptr
void mlp_free(char **list, int size) {
    for(int i = 0; i <= size; i++)
        ec_free(list[i]);

    ec_free((void *)list);
}

// Main craigslist link parser
int parse_cl_main(const char *input_file, const char *output_file, char **list) {
    // Check for html
    if(strstr(input_file, "html") == NULL) {
        fprintf(stderr, "Invalid html input file\n");
        return -1;
    }

    // Open input file
    int input_fd = ec_open(input_file, O_RDONLY);

    // Create buffer for html
    char *input_buffer = (char *)ec_malloc(sizeof(char *) * get_file_length(input_fd) + 32);
    
    // Copy html to buffer
    copy_to_memory(input_fd, input_buffer);

    // Close input file
    close(input_fd);

    // Remove output file
    remove(output_file);

    // Open output file
    int output_fd = ec_open(output_file, O_RDWR | O_CREAT);

    // Create ptr to traverse file
    char *ptr = input_buffer;

    // Find start index
    if((ptr = strstr(ptr, "<div class=\"box box_1\">")) == NULL) {
        fprintf(stderr, "Failed to find start index\n");
        return -1;
    }

    // Create buffer to hold link
    char *link = (char *)ec_malloc(sizeof(char) * 512);

    int line_count = 0;

    // Start main loop
    while((ptr = strstr(ptr, "href=\"")) != NULL) {
        ptr = ptr + strlen("href=\"");

        // Null out buffer
        memset(link, 0, 512);

        // Copy link
        int i = 0;
        for(; i < 512 && *ptr != '\"'; i++) {
            memset(link + i, *ptr, 1);
            ptr++;
        }
        memset(link + i, 0, 1);

        // if link data contains help break loop
        if(strstr(link, "help") != NULL)
            break;

        memcpy(list[line_count], link, i);

        printf("%s\n", list[line_count]);

        // Write link to file
        ec_write(output_fd, link, strlen(link));
        ec_write(output_fd, "\n", 1);

        line_count++;
    }

    // Cleanup
    ec_free(input_buffer);
    ec_free(link);
    close(output_fd);

    return line_count;
}

// Init memory library
void init_mem(void) {
    irc_mem_startup();
}

// Clean memory library
void clean_mem(void) {
    irc_mem_cleanup();
}

#endif