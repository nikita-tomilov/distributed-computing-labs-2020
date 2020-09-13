#ifndef PA_MAIN_H
#define PA_MAIN_H

#include <stdio.h>

#include "ipc.h"
#include "common.h"
#include "pa2345.h"
#include "banking.h"

#define LOG

typedef struct {
    local_id my_id;
    local_id max_id;
    int pipe_fd_from[MAX_PROCESS_ID + 1];
    int pipe_fd_to[MAX_PROCESS_ID + 1];

    int done_count;
    int cs;
    int forks[MAX_PROCESS_ID+1];
    int dirty[MAX_PROCESS_ID+1];
    int reqf[MAX_PROCESS_ID+1];
    int request_time;

    int last;

    unsigned int balance;
    int use_mutex;

    FILE* events_log_file;
    FILE* pipes_log_file;
} io_data;

#endif //PA_MAIN_H
