#ifndef PA_MAIN_H
#define PA_MAIN_H

#include <stdio.h>

#include "ipc.h"
#include "common.h"
#include "pa2345.h"
#include "banking.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef struct {
    local_id current_id;
    local_id max_id;
    int from[MAX_PROCESS_ID+1];
    int to[MAX_PROCESS_ID+1];

    int done;
    int queue[MAX_PROCESS_ID+1];
    int confirmation;

    int last;

    unsigned int balance;
    int mutexl;

    FILE* events;
    FILE* pipes;
} io_data;

#endif //PA_MAIN_H
