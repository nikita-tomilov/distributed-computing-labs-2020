#ifndef PA_LAMPORT_H
#define PA_LAMPORT_H

#include "banking.h"

timestamp_t inc_lamport_time();
void update_lamport_time(timestamp_t n);

#endif //PA_LAMPORT_H
