#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "banking.h"
#include "common.h"
#include "ipc.h"
#include "load.h"
#include "pa2345.h"
#include "transmission.h"

#include "log.h"

# define WEXITED	4	/* Report dead child.  */
# define WNOWAIT	0x01000000 /* Don't reap, just poll status.  */

extern int started_len;
extern int done_len;
extern char free_payload[MAX_PAYLOAD_LEN];

balance_t cash[11];
extern AllHistory *allhist;
extern timestamp_t lamport_time;

FILE *eventlog;
FILE *pipelog;
    
void child(int8_t num_processes, local_id id) {
    //reset money
    reset_balance_state(cash[id]);
    reset_balance_history(id);

    lamport_time = id - 1;
    process_send_multicast(id, STARTED);
    timestamp_t time = get_lamport_time();

    //log: process started
    llog(eventlog, log_started_fmt, time, id, getpid(), getppid(), get_balance());

    process_receive_all(num_processes, id, STARTED);
    time = get_lamport_time();
    //log: all received
    llog(eventlog, log_received_all_started_fmt, time, id);
    
    process_load(id);
        
    process_send_multicast(id, DONE);
    time = get_lamport_time();
    //log: done
    llog(eventlog, log_done_fmt, time, id, get_balance());

    process_receive_all(num_processes, id, DONE);
    time = get_lamport_time();
    //log: all done
    llog(eventlog, log_received_all_done_fmt, time, id);
    
    send_balance_history();
}

void spawn_childs(int8_t num_processes) {
    for (local_id i = 0; i < num_processes; i++) {
        pid_t child_pid = fork();
        if (child_pid == -1) {
            //this is fork error
            fail_gracefully("fork error");
        }
        if (child_pid == 0) {
            //this is child
            close_spare_pipes(num_processes, i + 1);
            child(num_processes, i + 1);
            close_used_pipes(num_processes, i + 1);
            //die. everything is fine
            fclose(eventlog);
            exit(EXIT_SUCCESS);
        }
        //this is parent; continue spawning childs
    }
}

void wait_for_childs(int8_t num_processes) {
    while (num_processes > 0) {
        __pid_t w = waitpid(-1, NULL, WEXITED || WNOHANG);
        if (w > 0) {
            num_processes--;
        } else if (w == -1) {
            fail_gracefully("wait");
        }
    }
}

void parent(int8_t num_processes, FILE *pipelog) {
    create_all_history(num_processes);
    create_pipe_matrix(num_processes, pipelog);
    spawn_childs(num_processes);
    close_spare_pipes(num_processes, PARENT_ID);
    process_receive_all(num_processes, PARENT_ID, STARTED);
    bank_robbery(NULL, num_processes);
    process_send_multicast(PARENT_ID, STOP);
    process_receive_all(num_processes, PARENT_ID, DONE);
    process_receive_all(num_processes, PARENT_ID, BALANCE_HISTORY);
    print_history(allhist);
    wait_for_childs(num_processes);
    close_used_pipes(num_processes, PARENT_ID);
}

int main(int argc, char *argv[]) {
    int num_processes;
    local_id i;
    
    // Launch params
    if(argc <= 3) {
        fprintf(stderr, "usage: %s -p numofprocesses <money1> [money2] ... [moneyN]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    num_processes = atoi(argv[2]);
    if (num_processes <= 0) {
        fprintf(stderr, "invalid input \"%s\"\n", argv[2]);
        fprintf(stderr, "usage: %s -p numofprocesses <money1> [money2] ... [moneyN]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    eventlog = fopen(events_log, "w");
    if (!eventlog) {
        fail_gracefully("fopen event_log");
    }
    pipelog = fopen(pipes_log, "w");
    if (!pipelog) {
        fail_gracefully("fopen pipes_log");
    }

    for (i = 0; i < num_processes; i++) {
        cash[i + 1] = atoi(argv[i + 3]);
    }

    if (!(pipelog = fopen(pipes_log, "w")) || !(eventlog = fopen(events_log, "a"))) {
        fail_gracefully("fopen");
    }
    //get msg size
    sprintf(free_payload, log_started_fmt, 32767, PARENT_ID, getpid(), getppid(), 99);
    started_len = strlen(free_payload);
    sprintf(free_payload, log_done_fmt, 32767, PARENT_ID, 99);
    done_len = strlen(free_payload);
    //go get the job done
    parent(num_processes, pipelog);
    fclose(eventlog);
    return 0;
}

