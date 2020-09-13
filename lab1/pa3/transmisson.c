#include "transmission.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "log.h"
#include "load.h"
#include "message.h"

extern FILE *eventlog;

char free_payload[MAX_PAYLOAD_LEN];
int pipefds_to_write[MAX_PIPES + 1][MAX_PIPES + 1];
int pipefds_to_read[MAX_PIPES + 1][MAX_PIPES + 1];

extern local_id receive_any_dst;

void create_pipe_matrix(int8_t num_processes, FILE *pipelog) {
    int res = 0;
    for (int i = 0; i < num_processes + 1; i++) {
        for (int j = 0; j < num_processes + 1; j++) {
        
            int pipefd[2];
            
            if (i == j) {
                pipefds_to_read[j][i] = -1;
                pipefds_to_write[i][j] = -1;
                continue;
            }
            
            res = pipe2(pipefd, O_NONBLOCK);
            if (res == -1) {
                fclose(pipelog);
                fclose(eventlog);
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            //pipefd[0] is for read
            //pipefd[1] is for write
            fprintf(pipelog, "%i %i\n", pipefd[0], pipefd[1]);
            pipefds_to_read[j][i] = pipefd[0];
            pipefds_to_write[i][j] = pipefd[1];
        }
        //borders
        pipefds_to_read[i][num_processes + 1] = -2;
        pipefds_to_write[i][num_processes + 1] = -2;
        pipefds_to_read[num_processes + 1][i] = -2;
        pipefds_to_write[num_processes + 1][i] = -2;
    }
    fclose(pipelog);
}

void close_spare_pipes(int8_t num_processes, local_id id) {
    for (int i = 0; i < num_processes + 1; i++) {
    
        if (i == id) continue;
        
        for (int j = 0; j < num_processes + 1; j++) {
            if (pipefds_to_read[i][j] != -1) {
                if (close(pipefds_to_read[i][j]) != 0) {
                    fail_gracefully("close_spare_pipes");
                }
                pipefds_to_read[i][j] = -1;
            }
            if (pipefds_to_write[i][j] != -1) {
                if (close(pipefds_to_write[i][j]) != 0) {
                    fail_gracefully("close_spare_pipes");
                }
                pipefds_to_write[i][j] = -1;
            }
        }
    }
}

void close_used_pipes(int8_t num_processes, local_id id) {
    for (int j = 0; j < num_processes + 1; j++) {
        if (pipefds_to_read[id][j] != -1) {
            if (close(pipefds_to_read[id][j]) != 0) {
                fail_gracefully("close pipes");
            }
            pipefds_to_read[id][j] = -1;
        }
        if (pipefds_to_write[id][j] != -1) {
            if (close(pipefds_to_write[id][j]) != 0) {
                fail_gracefully("close pipes");
            }
            pipefds_to_write[id][j] = -1;
        }
    }
}

void process_send_multicast(local_id id, int16_t type) {
    char *payload;
    Message *msg;
    payload = create_payload(type, id, NULL, NULL);
    msg = create_msg(type, payload, NULL);
    free(payload);
    if (send_multicast(pipefds_to_write[id], msg) != 0) {
        fail_gracefully("send_multicast");
    }
    count_sent_num(id, type);
    free(msg);
    msg = NULL;
}

void process_send(local_id from,
                  local_id to,
                  int16_t type,
                  TransferOrder *order,
                  BalanceHistory *history)
{
    char *payload;
    Message *msg;
    payload = create_payload(type, from, order, history);
    msg = create_msg(type, payload, history);
    free(payload);
    if (send(pipefds_to_write[from], to, msg) != 0) {
           fclose(eventlog);
           free(msg);
           perror("send_multicast");
           exit(EXIT_FAILURE);
    }
    count_sent_num(from, type);
    free(msg);
    msg = NULL;
 }


void process_receive_all(int8_t num_processes, local_id id, int16_t type) {
    Message *msg = NULL;
    int res;
    int8_t *rcvd_num;
    int *rcvd;
    
    int watchdog = 0;
    
    rcvd_num = get_rcvd_num(type);
    rcvd = get_rcvd(type);
    while(*rcvd_num < num_processes && watchdog < 1000) {
        for (int i = 1; i < num_processes + 1; i++) {
            msg = create_msg(-1, free_payload, NULL);
            if (rcvd[i] == 0) {
                res = receive(pipefds_to_read[id], i, msg);
                if (res == 0) {
                    process_msg(msg, id, i);
                } else if (res == -1) {
                    fail_gracefully("receive");
                }
            }
            free(msg);
            msg = NULL;
        }
        watchdog++;
    }
}

void process_receive_any(local_id id) {
    Message *msg = NULL;
    msg = create_msg(-1, free_payload, NULL);
    int res = receive_any(pipefds_to_read[id], msg);
    if (res == -1) {
        fail_gracefully("receive_any");
    }
   
    if (res == 0) {
        process_msg(msg, id, receive_any_dst);
    }

    free(msg);
    msg = NULL;
}

void process_load(local_id id) {
    load(id);
}
