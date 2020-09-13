#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include "main.h"
#include "ipc.h"
#include "banking.h"
#include "lamport.h"
#include "pa2345.h"

#ifdef SHOULD_LOG
#define LOG(format, ...) customlog(format, __VA_ARGS__)
#else
#define LOG(format, ...) ;
#endif

void customlog(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
    fflush(stdout);
    va_end(args);
}

int send_message(io_data *io, int dst, int type, timestamp_t time, char *payload, int size) {
    Message m;
    m.s_header.s_magic = MESSAGE_MAGIC;
    m.s_header.s_type = type;
    m.s_header.s_payload_len = size;
    m.s_header.s_local_time = time;
    memcpy(&m.s_payload, payload, size);

    if (dst != -1)
        return send(io, dst, &m);
    return send_multicast(io, &m);
}

int send_started_message(io_data *io) {
    char payload[MAX_PAYLOAD_LEN] = {0};
    timestamp_t time = inc_lamport_time();
    int size = sprintf(payload, log_started_fmt, time, io->my_id, getpid(), getppid(), io->balance);
    fprintf(stdout, "%s", payload);
    fflush(stdout);
    fprintf(io->events_log_file, "%s", payload);
    fflush(io->events_log_file);
    return send_message(io, -1, STARTED, time, payload, size);
}

int send_done_message(io_data *io) {
    char payload[MAX_PAYLOAD_LEN] = {0};
    timestamp_t time = inc_lamport_time();
    int size = sprintf(payload, log_done_fmt, time, io->my_id, io->balance);
    fprintf(stdout, "%s", payload);
    fflush(stdout);
    fprintf(io->events_log_file, "%s", payload);
    fflush(io->events_log_file);
    return send_message(io, -1, DONE, time, payload, size);
}

void wait_for_all(io_data *io, int type) {
    int reported[MAX_PROCESS_ID + 1] = {0};
    reported[io->my_id] = 2;

    int reported_count = 1;
    if (io->my_id != PARENT_ID)
        reported_count++;
    while (reported_count <= io->max_id) {
        for (int i = 1; i <= io->max_id; i++) {
            if (reported[i] != 0) continue;

            Message m;
            int result = receive(io, i, &m);
            if (result == 0) {
                if (m.s_header.s_type != type) continue;
                fflush(stdout);
                reported[i] = 1;
                reported_count++;
            }
        }
        sleep(0);
    }
}

void send_broadcast_and_wait_for_response(io_data *io, int type) {
    int result;
    if (type == STARTED) {
        result = send_started_message(io);
    } else if (type == DONE) {
        result = send_done_message(io);
    } else {
        return;
    }

    if (result != 0) {
        return;
    }

    wait_for_all(io, type);

    if (type == STARTED) {
        fprintf(stdout, log_received_all_started_fmt, get_lamport_time(), io->my_id);
        fflush(stdout);
        fprintf(io->events_log_file, log_received_all_started_fmt, get_lamport_time(), io->my_id);
        fflush(io->events_log_file);
    } else {
        fprintf(stdout, log_received_all_done_fmt, get_lamport_time(), io->my_id);
        fflush(stdout);
        fprintf(io->events_log_file, log_received_all_done_fmt, get_lamport_time(), io->my_id);
        fflush(io->events_log_file);
    }
}

void handle_done_message(io_data *io) {
    io->done_count++;
}

void handle_cs_request_message(io_data *io) {
    LOG("process %i: process %i wants fork from us", io->my_id, io->message_from_id);
    io->request_marker[io->message_from_id] = 1;
    if (io->fork_to_process_i_is_dirty[io->message_from_id] == 1) {
        io->fork_to_process_i_is_mine[io->message_from_id] = 0;
        io->fork_to_process_i_is_dirty[io->message_from_id] = 0;
        send_message(io, io->message_from_id, CS_REPLY, inc_lamport_time(), NULL, 0);
        io->request_marker[io->message_from_id] = 0;
        if (io->requested_cs) {
            send_message(io, io->message_from_id, CS_REQUEST, get_lamport_time(), NULL, 0);
            io->request_marker[io->message_from_id] = 0;
        }
    }
}

void handle_cs_reply_message(io_data *io) {
    LOG("process %i: process %i gave fork to us", io->my_id, io->message_from_id);
    io->fork_to_process_i_is_mine[io->message_from_id] = 1;
    io->fork_to_process_i_is_dirty[io->message_from_id] = 0;
}

void handle_message(io_data *io) {
    Message m;
    int result = receive_any(io, &m);
    if (result != 0) return;

    switch(m.s_header.s_type) {
        case DONE:
            handle_done_message(io);
            break;
        case CS_REQUEST:
            handle_cs_request_message(io);
            break;
        case CS_REPLY: {
            handle_cs_reply_message(io);
            break;
        }
    }
}

//methodichka, pages around 124
int request_cs(const void *self) {
    io_data *io = (io_data *) self;
    LOG("process %i: wants to enter critical section", io->my_id);

    io->requested_cs = 1;

    inc_lamport_time();
    for (int i = 1; i <= io->max_id; i++) {
        if (i == io->my_id) continue;
        if (io->fork_to_process_i_is_mine[i] == 0) {
            send_message(io, i, CS_REQUEST, get_lamport_time(), NULL, 0);
            io->request_marker[i] = 0;
            LOG("process %i: requests fork from %i", io->my_id, i);
        }
    }

    int total = 2;
    while (total <= io->max_id) {
        handle_message(io);

        total = 2;
        for (int i = 1; i <= io->max_id; i++) {
            if (i == io->my_id) continue;
            if (io->fork_to_process_i_is_mine[i] == 1) {
                if (io->fork_to_process_i_is_dirty[i] == 1 && io->request_marker[i] == 1) continue;
                total++;
            }
        }
    }

    LOG("process %i: entered critical section", io->my_id);
    return 0;
}

int release_cs(const void *self) {
    io_data *io = (io_data *) self;
    LOG("process %i: wants to leave critical section", io->my_id);

    io->requested_cs = 0;
    for (int i = 1; i <= io->max_id; i++) {
        if (i == io->my_id) continue;
        io->fork_to_process_i_is_dirty[i] = 1;
    }

    inc_lamport_time();
    for (int i = 1; i <= io->max_id; i++) {
        if (i == io->my_id) continue;
        if (io->request_marker[i] == 1) {
            io->fork_to_process_i_is_mine[i] = 0;
            send_message(io, i, CS_REPLY, get_lamport_time(), NULL, 0);
        }
    }

    return 0;
}
