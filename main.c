//
// Programmer:      Matthew Todd Geiger
// Creation Date:   Wed May     8 11:06:30 PST 2019
//
// Filename:        main.c
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
#include <wait.h>

// lib includes
#include "lib/irc_err.h"
#include "lib/irc_mem.h"
#include "lib/irc_fcntl.h"
#include "lib/irc_types.h"
#include "lib/irc_curl.h"
#include "lib/irc_cl.h"
#include "lib/irc_arpa.h"

// Curl includes
#include <curl/curl.h>

#define PORT 2112

typedef struct auto_ref {
    int argc;
    const char **argv;
} ARD, *pARD;

int wait_process() {
    int status = 0;
    pid_t w_pid = wait(&status);
    int we_status = WEXITSTATUS(status);

    if(w_pid < 0) {
        fprintf(stderr, "Error: wait()\n");
        return -1;
    } else
        return we_status;
}

void *auto_refresh(void *args)
{
    pARD data = (pARD)args;

    while (TRUE)
    {
        if (running == FALSE)
        {
            running = TRUE;
            // fork process
            pid_t d_pid = fork();
            if (d_pid < 0)
                error_exit("Failed to launch child process");

            if (d_pid == 0) {
                curl_global_init(CURL_GLOBAL_ALL);
                launch_data_miner((const int)data->argc, (const char **)data->argv);
                curl_global_cleanup();
            }

            int d_status = 0;
            pid_t wait_pid = waitpid(d_pid, &d_status, WNOHANG);
            if (wait_pid < 0)
                error_exit("FATAL: waitpid()");
            else if (wait_pid == 0)
            {
                int pd_status = wait_process();
                if (pd_status < 0)
                {
                    fprintf(stderr, "Data miner failed\n");
                }
            }

            printf("Scraper Terminated\n");
            running = FALSE;
        }

        sleep(1800);
    }

    pthread_exit(0);
}

void main_handler(const struct sockaddr_in *sin, int client_fd, const int argc, const char *argv[]) {
    char *recv_buffer = (char *)ec_malloc(sizeof(char) * 512);

    if(ec_recv(client_fd, recv_buffer, 512, 0) >= 512) {
        fprintf(stderr, "Recieved buffer is to large\n");
        ec_send(client_fd, "FAILED\0", strlen("FAILED") + 1, 0);

        ec_free(recv_buffer);
        return;
    }

    if(memcmp(recv_buffer, "GET", 3) != 0) {
        fprintf(stderr, "Invalid buffer recieved\n");
        ec_send(client_fd, "FAILED\0", strlen("FAILED") + 1, 0);

        ec_free(recv_buffer);
        return;
    }

    // fork process
    pid_t d_pid = fork();
    if(d_pid < 0)
        error_exit("Failed to launch child process");

    if(d_pid == 0) {
        curl_global_init(CURL_GLOBAL_ALL);
        launch_data_miner(argc, argv);
        curl_global_cleanup();
    }

    int d_status = 0;
    pid_t wait_pid = waitpid(d_pid, &d_status, WNOHANG);
    if(wait_pid < 0)
        error_exit("FATAL: waitpid()");
    else if(wait_pid == 0) {
        int pd_status = wait_process();
        if(pd_status < 0) {
            fprintf(stderr, "Data miner failed\n");
            ec_send(client_fd, "FAILED\0", strlen("FAILED") + 1, 0);

            ec_free(recv_buffer);
            return;
        }
    }

    printf("Scraper Terminated\n");
    ec_send(client_fd, "SUCCESS\0", strlen("SUCCESS") + 1, 0);

    ec_free(recv_buffer);
}

void launch_handler(const int argc, const char *argv[]) {
    struct sockaddr_in sin;

    // Create socket and bind information
    int sockfd = ec_socket(AF_INET, SOCK_STREAM, 0);

    create_sockaddrin(&sin, PORT, AF_INET, "127.0.0.1");

    ec_setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR);
    
    ec_bind(sockfd, (const struct sockaddr *)&sin, sizeof(sin));

    // Setup listener and handler
    ec_listen(sockfd, 5);

    int client_fd = 0;
    struct sockaddr_in client_sin;

    printf("Initialization complete\n");

    pthread_t tid;
    pthread_attr_t attr;

    pthread_attr_init(&attr);

    ARD thread_data = {0};
    thread_data.argc = argc;
    thread_data.argv = argv;

    pthread_create(&tid, &attr, auto_refresh, &thread_data);

    while((client_fd = ec_accept(sockfd, (struct sockaddr *)&client_sin, sizeof(sin))) >= 0) {
        if(running == FALSE) {
            running = TRUE;
            main_handler(&client_sin, client_fd, argc, argv);
            running = FALSE;
        }
        close(client_fd);
    }
}

int exec_node(char *args[]) {
    if(execvp("node", args) < 0)
        return EXIT_FAILURE;
    
    return EXIT_SUCCESS;
}

void launch_web_server() {
    pid_t s_pid = fork();
    if(s_pid < 0)
        error_exit("FATAL ERROR: Failed to launch web server");

    if(s_pid == 0) {
        printf("Child process launch initiating\n");

        char *args[] = {"node", "web_server/app.js", NULL};

        if(exec_node(args) == EXIT_FAILURE)
            error_exit("FATAL ERROR: Failed to launch node web server\n");
    } else if(s_pid > 0)
        printf("Parent Process intact...\n");
}

int main(const int argc, const char *argv[]) {
    init_mem();

    // launch web server
    //launch_web_server();

    // launch handler
    launch_handler(argc, argv);

    clean_mem();
    exit(EXIT_SUCCESS);
}

/*int wait_process() {
    int status = 0;
    pid_t w_pid = wait(&status);
    int we_status = WEXITSTATUS(status);

    if(w_pid < 0) {
        fprintf(stderr, "Error: wait()\n");
        return -1;
    } else
        return we_status;
}

int main(const int argc, const char *argv[]) {
    // fork process
    pid_t d_pid = fork();
    if(d_pid < 0)
        error_exit("Failed to launch child process");

    if(d_pid == 0)
        launch_data_miner(argc, argv);

    int d_status = 0;
    pid_t wait_pid = waitpid(d_pid, &d_status, WNOHANG);
    if(wait_pid < 0)
        error_exit("FATAL: waitpid()");
    else if(wait_pid == 0) {
        int pd_status = wait_process();
        if(pd_status < 0) {
            fprintf(stderr, "Data miner failed\n");
        }
    }

    printf("Process Terminated\n");

    exit(EXIT_SUCCESS);
}*/