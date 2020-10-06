#include "banking.h"
#include "ipc.h"
#include "main.h"
#include "message.h"
#include "lamport.h"
#include "string.h"

#include <unistd.h>
#include <stdlib.h>

void transfer(void *parent_data, local_id src, local_id dst, balance_t amount) {
    io_data *io = (io_data *) parent_data;

    init_transfer(io, src, dst, amount);

    Message m;
    int result = receive(io, dst, &m);
    while (result != 0) {
        sleep(0);
        result = receive(io, dst, &m);
    }

    if (m.s_header.s_type != ACK) {
        fprintf(stderr, "Parent get message with type %i instead of %i (ACK)\n", m.s_header.s_type, ACK);
    }
}

void wait_for_all_balance_states(io_data *io, unsigned int *total_balance) {
    int reported[MAX_PROCESS_ID + 1] = {0};
    reported[io->current_id] = 2;

    int reported_count = 1;
    if (io->current_id != PARENT_ID)
        reported_count++;
    while (reported_count <= io->max_id) {
        for (int i = 1; i <= io->max_id; i++) {
            if (reported[i] != 0) continue;

            Message m;
            int result = receive(io, i, &m);
            if ((result == 0) && (m.s_header.s_type == BALANCE_STATE)) {
                reported[i] = 1;
                m.s_payload[m.s_header.s_payload_len] = 0;
                //printf("ACHIEVED FROM %d: '%s'\n", i, m.s_payload);
                char *eptr;
                unsigned int balance = strtoul(m.s_payload, &eptr, 10);
                (*total_balance) += balance;
                reported_count++;
            }
        }
        sleep(1);
    }
}

void total_sum_snapshot(void *parent_data) {
    io_data *io = (io_data *) parent_data;
    //around page 82
    send_broadcast_and_wait_for_response(io, SNAPSHOT_VTIME, SNAPSHOT_ACK);
    inc_lamport_time(0);
    uint total_balance = 0;
    wait_for_all_balance_states(io, &total_balance);
    printf("[%s]: %u %d\n", get_lamport_time_string(io->max_id), total_balance, 0);
}
