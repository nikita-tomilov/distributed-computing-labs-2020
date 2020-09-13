#include "message.h"

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "banking.h"
#include "pa2345.h"
#include "load.h"

#include "log.h"

extern FILE *eventlog;
extern timestamp_t lamport_time;

int started[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int done[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int balance[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int8_t started_num = 0;
int8_t done_num = 0;
int8_t balance_num = 0;

int started_len = 0;
int done_len = 0;

void process_msg_started(Message *msg) {
    int id, pid, ppid, time, bal;
    msg->s_payload[msg->s_header.s_payload_len] = '\0';
    sscanf(msg->s_payload, log_started_fmt, &time, &id, &pid, &ppid, &bal);
    if (started[id] == 0) {
        started_num++;
    }
    started[id]++;
}

void process_msg_done(Message *msg) {
    int id, time, bal;
    msg->s_payload[msg->s_header.s_payload_len] = '\0';
    sscanf(msg->s_payload, log_done_fmt, &time, &id, &bal);
    if (done[id] == 0) {
        done_num++;
    }
    done[id]++;
}

void process_msg(Message *msg, local_id dst, local_id src)
{
    lamport_time++;
    if (lamport_time <= msg->s_header.s_local_time) {
        while (lamport_time <= msg->s_header.s_local_time) {
            lamport_time++;
        }
    }

    switch (msg->s_header.s_type)
    {
    case STARTED:
        process_msg_started(msg);
        break;
    case DONE:
        process_msg_done(msg);
        break;
    case STOP:
        process_msg_stop(msg);
        break;
    case TRANSFER:
        process_msg_transfer(msg, dst);
        break;
    case ACK:
        process_msg_ack(msg, src, dst);
        break;
    case BALANCE_HISTORY:
        process_balance_history(msg);
        break;
    default:
        fail_gracefully("unknown msg");
    }
}

int payload_size(int16_t type, BalanceHistory *history) {
    switch (type) {
    case STARTED:
        return started_len;
        break;
    case DONE:
        return done_len;
        break;
    case STOP:
        return 0;
        break;
    case TRANSFER:
        return (sizeof(TransferOrder));
        break;
    case ACK:
        return 0;
        break;
    case BALANCE_HISTORY:
        return (sizeof(local_id) +
                sizeof(uint8_t) +
                history->s_history_len*sizeof(BalanceState));
    default:
        return MAX_PAYLOAD_LEN;
    }
}

Message *create_msg(int16_t type, char *payload, BalanceHistory *history)
{
    uint16_t i;
    Message *msg;
    uint16_t payload_len;
    payload_len = payload_size(type, history);
    
    if ((payload == NULL) && (payload_len != 0)) {
        fail_gracefully("create_msg");
    }
    
    msg = malloc(sizeof(MessageHeader)+payload_len);
    if (msg == NULL) {
        fail_gracefully("create_msg malloc");
    }
    msg->s_header.s_magic = MESSAGE_MAGIC;
    msg->s_header.s_type = type;
    msg->s_header.s_payload_len = payload_len;
    
    if (type >= 0) {
        lamport_time++;
    }
    msg->s_header.s_local_time = get_lamport_time();

    for (i = 0; i < payload_len; i++) {
        msg->s_payload[i] = payload[i];
    }
    
    return msg;
}

void create_typed_payload(int16_t type,
                          local_id id,
                          TransferOrder *order,
                          BalanceHistory *history,
                          char *payload,
                          uint16_t payload_len) {
    switch (type) {
    case STARTED:
        sprintf(payload,
                log_started_fmt,
                get_lamport_time() + 1,
                id,
                getpid(),
                getppid(),
                get_balance());
        break;
    case DONE:
        sprintf(payload,
                log_done_fmt,
                get_lamport_time() + 1,
                id,
                get_balance());
        break;
    case TRANSFER:
        if (order == NULL) {
            fail_gracefully("create_payload");
        }
        memcpy(payload, order, sizeof(TransferOrder));
        break;
    case BALANCE_HISTORY:
        if (history == NULL) {
            fail_gracefully("create_payload");
        }
        memcpy(payload, history, payload_len);
        break;
    }
}

char *create_payload(int16_t type,
                     local_id id,
                     TransferOrder *order,
                     BalanceHistory *history) {
    int payload_len = payload_size(type, history);
    char *payload = malloc(payload_len);
    if ((payload == NULL) && (payload_len != 0)) {
        fail_gracefully("create_payload malloc");
    }
    create_typed_payload(type, id, order, history, payload, payload_len);
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
    case BALANCE_HISTORY:
        balance[id]++;
        balance_num++;
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
    case BALANCE_HISTORY:
        return &balance_num;
        break;
    default:
        fail_gracefully("unknown msg");
    }
    return 0;
}

int *get_rcvd(int16_t type) {
    switch (type) {
    case STARTED:
        return started;
        break;
    case DONE:
        return done;
        break;
    case BALANCE_HISTORY:
        return balance;
        break;
    default:
        fail_gracefully("unknown msg");
    }
    return 0;
}
