#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "main.h"
#include "ipc.h"
#include "banking.h"
#include "lamport.h"

int send_message(io_data* io, int dst, int type, timestamp_t* time_vector, char* payload, int size) {
    Message m;
    m.s_header.s_magic = MESSAGE_MAGIC;
    m.s_header.s_type = type;
    m.s_header.s_payload_len = size;
    assign_lamport_time(time_vector, m.s_header.s_local_timevector);
    memcpy(&m.s_payload, payload, size);

    if(dst != -1)
        return send(io, dst, &m);
    return send_multicast(io, &m);
}

int send_started_message(io_data* io) {
    char payload[MAX_PAYLOAD_LEN] = {0};
    timestamp_t* time = inc_lamport_time();
    int size = sprintf(payload, log_started_fmt, get_my_lamport_time(io->current_id), io->current_id, getpid(), getppid(), io->balance);
    fprintf(stdout, "%s", payload);
    fflush(stdout);
    fprintf(io->events, "%s", payload);
    fflush(io->events);
    return send_message(io, -1, STARTED, time, payload, size);
}

int send_done_message(io_data* io) {
    char payload[MAX_PAYLOAD_LEN] = {0};
    timestamp_t* time = inc_lamport_time();
    int size = sprintf(payload, log_done_fmt, get_my_lamport_time(io->current_id), io->current_id, io->balance);
    fprintf(stdout, "%s", payload);
    fflush(stdout);
    fprintf(io->events, "%s", payload);
    fflush(io->events);
    return send_message(io, -1, DONE, time, payload, size);
}

void wait_for_all(io_data* io, int type) {
    int reported[MAX_PROCESS_ID+1] = {0};
    reported[io->current_id] = 2;

    int reported_count = 1;
    if(io->current_id != PARENT_ID)
        reported_count ++;
    while (reported_count <= io->max_id) {
        for(int i = 1; i <= io->max_id; i++) {
            if(reported[i] != 0) continue;

            Message m;
            int result = receive(io, i, &m);
            if(result == 0) {
                reported[i] = 1;
                reported_count++;
            }
        }
        sleep(1);
    }
}

void send_broadcast_and_wait_for_response(io_data* io, int type) {
    int result;
    if(type == STARTED) {
        result = send_started_message(io);
    } else if(type == DONE) {
        result = send_done_message(io);
    } else {
        return;
    }

    if(result != 0) {
        return;
    }

    wait_for_all(io, type);

    if(type == STARTED) {
        fprintf(stdout, log_received_all_started_fmt, get_my_lamport_time(io->current_id), io->current_id);
        fflush(stdout);
        fprintf(io->events, log_received_all_started_fmt, get_my_lamport_time(io->current_id), io->current_id);
        fflush(io->events);
    } else {
        fprintf(stdout, log_received_all_done_fmt, get_my_lamport_time(io->current_id), io->current_id);
        fflush(stdout);
        fprintf(io->events, log_received_all_done_fmt, get_my_lamport_time(io->current_id), io->current_id);
        fflush(io->events);
    }
}

int init_transfer(io_data* io, local_id src, local_id dst, int amount) {
    TransferOrder payload = {
            .s_src = src,
            .s_dst = dst,
            .s_amount = amount
    };
    timestamp_t* time = inc_lamport_time();
    int size = sizeof(TransferOrder);
    return send_message(io, src, TRANSFER, time, (char*)&payload, size);
}

int send_ack(io_data* io, local_id dst) {
    return send_message(io, dst, ACK, inc_lamport_time(), NULL, 0);
}

int send_broadcast_stop(io_data* io) {
    return send_message(io, -1, STOP, inc_lamport_time(), NULL, 0);
}
