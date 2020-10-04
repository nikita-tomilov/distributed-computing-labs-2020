#include "lamport.h"
#include "stdio.h"

timestamp_t lamport_time[MAX_PROCESS_ID + 1] = {0};
char buf[16384];

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

timestamp_t* get_lamport_time() {
    return lamport_time;
}

void assign_lamport_time(const timestamp_t* source, timestamp_t* dest) {
    for (int i = 0; i < MAX_PROCESS_ID; i++) {
        dest[i] = source[i];
    }
}

timestamp_t *inc_lamport_time() {
    for (int i = 0; i < MAX_PROCESS_ID; i++) {
        lamport_time[i]++;
    }
    return lamport_time;
}

void update_lamport_time(const timestamp_t *time_msg) {
    for (int i = 0; i < MAX_PROCESS_ID; i++) {
        lamport_time[i] = MAX(lamport_time[i], time_msg[i]);
    }
}

char *get_lamport_time_string(int n) {
    int index = 0;
    for (int i = 0; i <= n; i++)
        index += sprintf(&buf[index], "%d ", lamport_time[i]);
    return buf;
}

timestamp_t get_my_lamport_time(int my_id) {
    return lamport_time[my_id - 1];
}
