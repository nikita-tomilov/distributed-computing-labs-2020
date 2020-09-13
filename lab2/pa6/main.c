#define _GNU_SOURCE

#include "main.h"
#include "message.h"
#include "banking.h"

#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <getopt.h>

void close_all_except(io_data *io, int io_data_count, int except_for_this) {
    for (int i = 0; i < io_data_count; i++) {
        if (i == except_for_this) continue;
        for (int j = 0; j < io_data_count; j++) {
            close(io[i].pipe_fd_from[j]);
            close(io[i].pipe_fd_to[j]);
        }
    }
}

void log_pipes(io_data *io) {
    for (int i = 0; i <= io->max_id; i++) {
        fprintf(io->pipes_log_file, "%i pipe_fd_from %i = %i\n", io->my_id, i, io->pipe_fd_from[i]);
    }
    for (int i = 0; i <= io->max_id; i++) {
        fprintf(io->pipes_log_file, "%i pipe_fd_to %i = %i\n", io->my_id, i, io->pipe_fd_to[i]);
    }
    fflush(io->pipes_log_file);
}

void parse_args(int argc, char *argv[], int *p, int *use_mutex) {
    const char *short_options = "p:";

    const struct option long_options[] = {
            {"mutexl", no_argument, use_mutex, 1},
            {NULL, 0, NULL,                    0}
    };

    int arg = getopt_long(argc, argv, short_options, long_options, NULL);
    while (arg != -1) {
        if (arg == 'p') {
            *p = strtoimax(optarg, NULL, 10);
        }
        arg = getopt_long(argc, argv, short_options, long_options, NULL);
    }
    if (*p == -1) {
        fprintf(stderr, "usage: %s -p <numofprocesses> [--mutexl]\n", argv[0]);
        _exit(1);
    }
}

void create_io_structures(io_data *io, int p, FILE *events_file, FILE *pipes_file, int use_mutex) {
    for (int i = 0; i <= p; i++) {
        io[i].my_id = i;
        io[i].max_id = p;

        io[i].events_log_file = events_file;
        io[i].pipes_log_file = pipes_file;

        io[i].balance = 0;

        io[i].use_mutex = use_mutex;
        io[i].done_count = 2;
        io[i].request_time = 100000;
        io[i].cs = 0;

        for (int j = 0; j <= p; j++) {
            io[i].forks[j] = (j >= i) ? 1 : 0;
            io[i].dirty[j] = io[i].forks[j];
            io[i].reqf[j] = 1 - io[i].forks[j];
            if (i == j) {
                io[i].pipe_fd_to[i] = -1;
                io[i].pipe_fd_from[i] = -1;
                continue;
            }
            int fd[2];
            pipe2(fd, O_NONBLOCK);
            io[i].pipe_fd_to[j] = fd[1];
            io[j].pipe_fd_from[i] = fd[0];
        }
    }
}

int child_loop(io_data io) {
    //step one - notify that I am ready
    send_broadcast_and_wait_for_response(&io, STARTED);
    //step two - actual prints
    for (int i = 1; i <= io.my_id * 5; i++) {
        if (io.use_mutex) {
            request_cs(&io);
        }
        char buf[100];
        sprintf(buf, log_loop_operation_fmt, io.my_id, i, io.my_id * 5);
        print(buf);
        if (io.use_mutex) {
            release_cs(&io);
        }
    }
    //step three - notify that I am done
    send_done_message(&io);
    //step four - wait for others
    while (io.done_count <= io.max_id) {
        handle_message(&io);
    }
    return 0;
}

int parent_loop(io_data *io, int p) {
    close_all_except(io, p + 1, PARENT_ID);

    wait_for_all(io, STARTED);
    wait_for_all(io, DONE);

    for (int i = 1; i <= p; i++) {
        wait(NULL);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int p = -1;
    int use_mutex = 0;
    parse_args(argc, argv, &p, &use_mutex);

    FILE *events = fopen(events_log, "w");
    FILE *pipes = fopen(pipes_log, "w");
    io_data io[p + 1];
    create_io_structures(io, p, events, pipes, use_mutex);

#ifdef LOG
    for (int i = 0; i < p; i++) {
        log_pipes(&io[i]);
    }
#endif

    for (int i = 1; i <= p; i++) {
        int pid = fork();
        if (pid == 0) {
            //do the child duty
            close_all_except(io, p + 1, i);
            return child_loop(io[i]);
        } else {
            //continue spawning childs
        }
    }
    return parent_loop(io, p);
}
