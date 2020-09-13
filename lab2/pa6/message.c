#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "main.h"
#include "ipc.h"
#include "banking.h"
#include "lamport.h"
#include "pa2345.h"

int send_message(io_data* io, int dst, int type, timestamp_t time, char* payload, int size) {
    Message m;
    m.s_header.s_magic = MESSAGE_MAGIC;
    m.s_header.s_type = type;
    m.s_header.s_payload_len = size;
    m.s_header.s_local_time = time;
    memcpy(&m.s_payload, payload, size);

    if(dst != -1)
        return send(io, dst, &m);
    return send_multicast(io, &m);
}

int send_started_message(io_data* io) {
    char payload[MAX_PAYLOAD_LEN] = {0};
    timestamp_t time = inc_lamport_time();
    int size = sprintf(payload, log_started_fmt, time, io->current_id, getpid(), getppid(), io->balance);
    fprintf(stdout, "%s", payload);
    fflush(stdout);
    fprintf(io->events, "%s", payload);
    fflush(io->events);
    return send_message(io, -1, STARTED, time, payload, size);
}

int send_done_message(io_data* io) {
    char payload[MAX_PAYLOAD_LEN] = {0};
    timestamp_t time = inc_lamport_time();
    int size = sprintf(payload, log_done_fmt, time, io->current_id, io->balance);
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
//                fprintf(stdout, "Process %i reported about %i == %i\n", i, m.s_header.s_type, type);
                if(m.s_header.s_type != type) continue;
                fflush(stdout);
                reported[i] = 1;
                reported_count++;
            }
        }
        sleep(0);
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
        fprintf(stdout, log_received_all_started_fmt, get_lamport_time(), io->current_id);
        fflush(stdout);
        fprintf(io->events, log_received_all_started_fmt, get_lamport_time(), io->current_id);
        fflush(io->events);
    } else {
        fprintf(stdout, log_received_all_done_fmt, get_lamport_time(), io->current_id);
        fflush(stdout);
        fprintf(io->events, log_received_all_done_fmt, get_lamport_time(), io->current_id);
        fflush(io->events);
    }
}

void handle_message(io_data* io) {
    Message m;
    int result = receive_any(io, &m);
    if(result != 0) return;

    if(m.s_header.s_type == DONE) {
        io->done++;
    } else if(m.s_header.s_type == CS_REQUEST) {
        io->reqf[io->last] = 1;
        if(io->dirty[io->last] == 1) {
            io->forks[io->last] = 0;
            io->dirty[io->last] = 0;
            send_message(io, io->last, CS_REPLY, inc_lamport_time(), NULL, 0);
            io->reqf[io->last] = 0;
            if(io->cs) {
                send_message(io, io->last, CS_REQUEST, get_lamport_time(), NULL, 0);
                io->reqf[io->last] = 0;
            }
        }
    } else if(m.s_header.s_type == CS_REPLY) {
        io->forks[io->last] = 1;
        io->dirty[io->last] = 0;
//        fprintf(stdout, "process %i: process %i отдал нам вилку\n", io->current_id, io->last);
//        fflush(stdout);
    }

//    fprintf(stdout, "Process %i queue: ", io->current_id);
//    for(int i = 0; i <= io->max_id; i++) {
//        fprintf(stdout, "%i ", io->queue[i]);
//    }
//    fprintf(stdout, "\n");
//    fflush(stdout);
}

int request_cs(const void * self) {
    io_data* io = (io_data*)self;
//    fprintf(stdout, "process %i хочет войти в cs\n", io->current_id);
//    fflush(stdout);

    io->cs = 1;

    inc_lamport_time();
    for(int i = 1; i <= io->max_id; i++) {
        if(i == io->current_id) continue;
        if(io->forks[i] == 0) {
            send_message(io, i, CS_REQUEST, get_lamport_time(), NULL, 0);
            io->reqf[i] = 0;
//            fprintf(stdout, "process %i запрашивает вилку у %i\n", io->current_id, i);
//            fflush(stdout);
        }
    }

    int total = 2;
    while(total <= io->max_id) {
        handle_message(io);

        total = 2;
        for(int i = 1; i <= io->max_id; i++) {
            if(i == io->current_id) continue;
            if(io->forks[i] == 1) {
                if(io->dirty[i] == 1 && io->reqf[i] == 1) continue;
                total++;
            }
        }
    }

//    fprintf(stdout, "process %i entered cs\n", io->current_id);
//    fflush(stdout);
    return 0;
}

int release_cs(const void * self) {
    io_data* io = (io_data*)self;
//    fprintf(stdout, "process %i leave cs\n", io->current_id);
//    fflush(stdout);

    io->cs = 0;
    for(int i = 1; i <= io->max_id; i++) {
        if(i == io->current_id) continue;
        io->dirty[i] = 1;
    }

    inc_lamport_time();
    for(int i = 1; i <= io->max_id; i++) {
        if(i == io->current_id) continue;
        if(io->reqf[i] == 1) {
            io->forks[i] = 0;
            send_message(io, i, CS_REPLY, get_lamport_time(), NULL, 0);
        }
    }

    return 0;
}
