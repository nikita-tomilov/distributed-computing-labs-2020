#include "banking.h"
#include "ipc.h"
#include "main.h"
#include "message.h"
#include "lamport.h"

#include <unistd.h>

void transfer(void * parent_data, local_id src, local_id dst, balance_t amount) {
    io_data* io = (io_data*)parent_data;

    init_transfer(io, src, dst, amount);

    Message m;
    int result = receive(io, dst, &m);
    while(result != 0) {
        sleep(0);
        result = receive(io, dst, &m);
    }

    if(m.s_header.s_type != ACK) {
        fprintf(stderr, "Parent get message with type %i instead of %i (ACK)\n", m.s_header.s_type, ACK);
    }
}

void total_sum_snapshot(void * parent_data)
{
    io_data* io = (io_data*)parent_data;
    printf("PARENT Time [%s]\n", get_lamport_time_string(io->max_id));
    //around page 182
    //TODO: implement
}
