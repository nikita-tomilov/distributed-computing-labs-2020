#ifndef PA_MESSAGE_H
#define PA_MESSAGE_H

#include "main.h"

void send_broadcast_and_wait_for_response(io_data* io, int type);

void wait_for_all(io_data* io, int type);
int init_transfer(io_data* io, local_id src, local_id dst, int amount);
int send_ack(io_data* io, local_id dst);
int send_broadcast_stop(io_data* io);

int send_history(io_data* io);
int send_done_message(io_data* io);

void total_sum_snapshot(void * parent_data);

#endif //PA_MESSAGE_H
