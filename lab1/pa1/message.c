#include "message.h"

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "pa1.h"

#include "log.h"

extern FILE *eventlog;

int started[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int done[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int8_t started_num = 0;
int8_t done_num = 0;

int started_len = 0;
int done_len = 0;

void process_msg_started(Message *msg) {
    int id, pid, ppid;
    msg->s_payload[msg->s_header.s_payload_len] = '\0';
    sscanf(msg->s_payload, log_started_fmt, &id, &pid, &ppid);
    if (started[id] == 0) started_num++;
    started[id]++;
}

void process_msg_done(Message *msg) {
    int id;
    msg->s_payload[msg->s_header.s_payload_len] = '\0';
    sscanf(msg->s_payload, log_done_fmt, &id);
    if (done[id] == 0) done_num++;
    done[id]++;
}

void process_msg(Message *msg) {
    switch (msg->s_header.s_type) {
        case STARTED:
            process_msg_started(msg);
            break;
        case DONE:
            process_msg_done(msg);
            break;
        default:
            fail_custom("process_msg: unknown message type");    
    }
}

int payload_size(int16_t type) {
    switch (type) {
        case STARTED:
            return started_len;
            break;
        case DONE:
            return done_len;
            break;
        default:
            return MAX_PAYLOAD_LEN;
    }
}

Message* create_msg(int16_t type, char *payload) {
    uint16_t i;
    Message *msg;
    uint16_t payload_len;
    payload_len = payload_size(type);
    if (payload == NULL) {
        fail_custom("create_msg: payload is null");
    }
    msg = malloc(sizeof(MessageHeader)+payload_len);
    if (msg == NULL) {
        fail_gracefully("create_msg");
    }
    msg->s_header.s_magic = MESSAGE_MAGIC;
    msg->s_header.s_type = type;
    msg->s_header.s_payload_len = payload_len;
    msg->s_header.s_local_time = time(NULL);
    for (i = 0; i < payload_len; i++) {
        msg->s_payload[i] = payload[i];
    }
    return msg;
}

const char* log_fmt_type(int16_t type) {
    switch (type) {
        case STARTED:
            return log_started_fmt;
            break;
        case DONE:
            return log_done_fmt;
            break;
        default:
            fail_custom("log_fmt_type: unknown message type");    
    }
    return NULL;
}

char* create_payload(int16_t type, local_id id) {
    char* payload = malloc(payload_size(type));
    if (payload == NULL) {
        fail_gracefully("create_payload");
    }
    sprintf(payload, log_fmt_type(type), id, getpid(), getppid());
    return payload;
}

void count_sent_num(local_id id, int16_t type) {
    switch (type) {
        case STARTED:
            started[id]++;
            started_num++;
            break;
        case DONE:
            done[id]++;
            done_num++;
            break;
        default:
            fail_custom("count_sent_num: unknown message type"); 
    }
}

int8_t *get_rcvd_num(int16_t type) {
    switch (type) {
        case STARTED:
            return &started_num;
            break;
        case DONE:
            return &done_num;
            break;
        default:
            fail_custom("get_rcvd_num: unknown message type"); 
    }
    return NULL;
}

int *get_rcvd(int16_t type) {
    switch (type) {
        case STARTED:
            return started;
            break;
        case DONE:
            return done;
            break;
        default:
            fail_custom("get_rcvd: unknown message type"); 
    }
    return NULL;
}
