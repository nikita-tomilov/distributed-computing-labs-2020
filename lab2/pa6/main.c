#define _GNU_SOURCE

#include "main.h"
#include "message.h"
#include "banking.h"

#include "lamport.h"

#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <getopt.h>

void close_all_except(io_data* io, int count, int id) {
    for(int i = 0; i < count; i++) {
        if(i == id) continue;
        for(int j = 0; j < count; j++) {
            close(io[i].from[j]);
            close(io[i].to[j]);
        }
    }
}

io_data* s;

void log_pipes(io_data* io) {
    /* Логгируем все пайпы*/
    for(int i = 0; i <= io->max_id; i++){
        fprintf(io->pipes, "%i from %i = %i\n", io->current_id, i, io->from[i]);
    }
    for(int i = 0; i <= io->max_id; i++){
        fprintf(io->pipes, "%i to %i = %i\n", io->current_id, i, io->to[i]);
    }
    fflush(io->pipes);
}

int child_loop(io_data io) {
    s = &io;
    /* Первый этап */
    send_broadcast_and_wait_for_response(&io, STARTED);
    /* Второй этап, полезная работа */
    for(int i = 1; i <= io.current_id * 5; i++) {
        if(io.mutexl) {
            request_cs(&io);
        }
        char buf[100];
        sprintf(buf, log_loop_operation_fmt, io.current_id, i, io.current_id * 5);
        print(buf);
        if(io.mutexl) {
            release_cs(&io);
        }
    }

    send_done_message(&io);

    while(io.done <= io.max_id) {
        handle_message(&io);
    }
    return 0;
}

void handler(int sig) {
    fprintf(stdout, "forks: ");
    for(int i = 1; i <= s->max_id; i++) {
        fprintf(stdout, "%i ", s->forks[i]);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "dirty: ");
    for(int i = 1; i <= s->max_id; i++) {
        fprintf(stdout, "%i ", s->dirty[i]);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "reqf: ");
    for(int i = 1; i <= s->max_id; i++) {
        fprintf(stdout, "%i ", s->reqf[i]);
    }
    fprintf(stdout, "\n");
    fflush(stdout);
}

int main(int argc, char* argv[]) {
    int p = -1;
    int mutexl = 0;

    signal(SIGUSR1, handler);

    /* Разбираем аргументы */
    const char* short_options = "p:";

    const struct option long_options[] = {
            {"mutexl", no_argument, &mutexl,1},
            {NULL,0,NULL,0}
    };

    int arg = getopt_long(argc,argv,short_options, long_options,NULL);
    while (arg != -1) {
        if(arg == 'p') {
            p = strtoimax(optarg, NULL, 10);
        }
        arg = getopt_long(argc,argv,short_options, long_options,NULL);
    }

    if(p == -1) {
        fprintf(stderr, "set processes count with -p\n");
        return 1;
    }

    /* Создаём io-структуры, которые раздадим детям */
    FILE* events = fopen(events_log, "w");
    FILE* pipes = fopen(pipes_log, "w");
    io_data io[p+1];
    for(int i = 0; i <= p; i++) {
        io[i].current_id = i;
        io[i].max_id = p;

        io[i].events = events;
        io[i].pipes = pipes;

//        if(i == 0) {
            io[i].balance = 0;
//        } else {
//            io[i].balance = strtoimax(argv[optind + i - 1], NULL, 10);
//        }

        io[i].mutexl = mutexl;
        io[i].done = 2;
        io[i].request_time = 100000;
        io[i].cs = 0;

        for(int j = 0; j <= p; j++) {
            io[i].forks[j] = (j >= i) ? 1 : 0;
            io[i].dirty[j] = io[i].forks[j];
            io[i].reqf[j] = 1 - io[i].forks[j];
            if(i == j) {
                io[i].to[i] = -1;
                io[i].from[i] = -1;
                continue;
            }
            int fd[2];
            pipe2(fd, O_NONBLOCK);
            io[i].to[j] = fd[1];
            io[j].from[i] = fd[0];
        }
    }

    /* Запускаем детей */
    for(int i = 1; i <= p; i++) {
        int pid = fork();
        if(pid == 0) {
            /* Ребёнок */
            close_all_except(io, p+1, i);
            return child_loop(io[i]);
        } else {
            /* Родитель */
        }
    }

    close_all_except(io, p+1, PARENT_ID);

    wait_for_all(io, STARTED);
    wait_for_all(io, DONE);

    for(int i = 1; i <= p; i++) {
        wait(NULL);
    }

    return 0;
}
