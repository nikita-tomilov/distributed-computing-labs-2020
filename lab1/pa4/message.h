#ifndef PA_MESSAGE_H
#define PA_MESSAGE_H

#include "main.h"

void send_broadcast_and_wait_for_response(io_data* io, int type);

void wait_for_all(io_data* io, int type);
int send_done_message(io_data* io);
void handle_message(io_data* io);

#endif //PA_MESSAGE_H
