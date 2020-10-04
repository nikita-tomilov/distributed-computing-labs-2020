#ifndef PA_LAMPORT_H
#define PA_LAMPORT_H

#include "banking.h"

timestamp_t*  get_lamport_time();
void assign_lamport_time(const timestamp_t* source, timestamp_t* dest);
timestamp_t* inc_lamport_time(int my_id);
void update_lamport_time(const timestamp_t* time_msg);
char* get_lamport_time_string(int n);
timestamp_t get_my_lamport_time(int my_id);

#endif //PA_LAMPORT_H
