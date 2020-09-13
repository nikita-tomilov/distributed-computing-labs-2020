#include "ipc.h"
#include "main.h"
#include "lamport.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>

char* type_from_int(int type) {
    switch (type) {
        case STARTED:
            return "STARTED";
        case DONE:
            return "DONE";
        case CS_REQUEST:
            return "CS_REQUEST";
        case CS_REPLY:
            return "CS_REPLY";
        case CS_RELEASE:
            return "CS_RELEASE";
        default:
            return "UNKNOWN";
    }
}

int send(void * self, local_id dst, const Message * msg) {
    io_data* io = (io_data*)self;

    /* Пишем сообщение */
    int bytes = write(io->to[dst], msg, sizeof(MessageHeader) + msg->s_header.s_payload_len);
    if(bytes == -1) {
        fprintf(stderr, "Process %i failed to send message to %i: %s\n", io->current_id, dst, strerror(errno));
        return 1;
    }

//    printf("Time %i: Process %i sent msg to %i (type %s)\n", get_lamport_time(), io->current_id, dst, type_from_int(msg->s_header.s_type));

    return 0;
}

int send_multicast(void * self, const Message * msg) {
    io_data* io = (io_data*)self;
    for(int i = 0; i <= io->max_id; i++) {
        if(i == io->current_id) continue;
        if(send(self, i, msg) != 0) {
            return 1;
        }
    }
    return 0;
}

int receive(void * self, local_id from, Message * msg) {
    io_data* io = (io_data*)self;
    MessageHeader h;
    int bytes = read(io->from[from], &h, sizeof(MessageHeader));
    if(bytes < 1) {
        return 1;
    }

    msg->s_header = h;

    update_lamport_time(msg->s_header.s_local_time);
    inc_lamport_time();
//    printf("Time %i: Process %i received msg from %i (type %s)\n", get_lamport_time(), io->current_id, from, type_from_int(h.s_type));
    if(h.s_payload_len == 0) {
        return 0;
    }
    bytes = read(io->from[from], msg->s_payload, h.s_payload_len);
    if(bytes == -1) {
        return 1;
    }

    return 0;
}

int receive_any(void * self, Message * msg) {
    io_data* io = (io_data*)self;
    for(int i = 0; i <= io->max_id; i++) {
        if(i == io->current_id) continue;
        if(receive(self, i, msg) == 0) {
            io->last = i;
            return 0;
        }
    }
    return 1;
}
