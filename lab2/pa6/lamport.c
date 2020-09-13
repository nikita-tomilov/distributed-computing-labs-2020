#include "lamport.h"

timestamp_t lamport_time = 0;

timestamp_t get_lamport_time() {
    return lamport_time;
}

timestamp_t inc_lamport_time() {
    lamport_time++;
    return lamport_time;
}

void update_lamport_time(timestamp_t n) {
    if(n > lamport_time)
        lamport_time = n;
}
