#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "common.h"
#include "ipc.h"
#include "pa1.h"
#include "transmission.h"
#include "log.h" 

# define WEXITED	4	/* Report dead child.  */
# define WNOWAIT	0x01000000 /* Don't reap, just poll status.  */

extern int started_len;
extern int done_len;
extern char free_payload[MAX_PAYLOAD_LEN];

FILE *eventlog;
FILE *pipelog;

//What child processes do
void child(int8_t num_processes, local_id id) {
    //log: process started
    llog(eventlog, log_started_fmt, id, getpid(), getppid());
    //widespread send and widespread receive
    process_send_multicast(id, STARTED);
    process_receive_all(num_processes, id, STARTED);
    //log: all received
    llog(eventlog, log_received_all_started_fmt, id);
    //do some job
    process_load();
    //log: process has done its job
    llog(eventlog, log_done_fmt, id);
    //widespread send and widespread receive
    process_send_multicast(id, DONE);
    process_receive_all(num_processes, id, DONE);
    //log: everything done
    llog(eventlog, log_received_all_done_fmt, id);
}

//Spawning those childs
void spawn_childs(int8_t num_processes) {
    for (int i = 0; i < num_processes; i++) {
        int16_t child_pid;
        child_pid = fork();
        if (child_pid == -1) {
            //this is the error while forking
            fail_gracefully("fork");
        }
        if (child_pid == 0) {
            //this is the child process
            close_spare_pipes(num_processes, i + 1);
            child(num_processes, i + 1);
            close_used_pipes(num_processes, i + 1);
            //die. everything is fine
            fclose(eventlog);
            exit(EXIT_SUCCESS);
        }
        //this is the parent process. continue spawning childs
    }
}

//Waiting for childs and receiving messages from them
void wait_for_childs(int8_t num_processes) {
    while (num_processes > 0) {
        int16_t w;
        w = waitpid(-1, NULL, WEXITED || WNOHANG);
        if (w > 0) {
            num_processes--;
        }
        if (w == -1) {
            fail_gracefully("waitpid");
        }
        process_receive_any(PARENT_ID);
    }
}

void parent(int8_t num_processes, FILE *pipelog)
{
    create_pipe_matrix(num_processes, pipelog);
    spawn_childs(num_processes);
    close_spare_pipes(num_processes, PARENT_ID);
    wait_for_childs(num_processes);
    close_used_pipes(num_processes, PARENT_ID);
}

int main(int argc, char *argv[])
{
    int num_processes = 0;

    // Launch params
    if(argc != 3) {
        fprintf(stderr, "usage: %s -p numofprocesses\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    num_processes = atoi(argv[2]);
    if (num_processes <= 0) {
        fprintf(stderr, "invalid input \"%s\"\n", argv[2]);
        fprintf(stderr, "usage: %s -p numofprocesses\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    eventlog = fopen(events_log, "a");
    if (!eventlog) {
        fail_gracefully("fopen event_log");
    }
    pipelog = fopen(pipes_log, "w");
    if (!pipelog) {
        fail_gracefully("fopen pipes_log");
    }

    // Defining message size
    sprintf(free_payload, log_started_fmt, PARENT_ID, getpid(), getppid());
    started_len = strlen(free_payload);
    sprintf(free_payload, log_done_fmt, PARENT_ID);
    done_len = strlen(free_payload);
    
    // Doing parent job
    parent(num_processes, pipelog);
    
    
    fclose(eventlog);
    return 0;
}

