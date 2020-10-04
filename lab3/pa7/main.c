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

void close_all_except(io_data *io, int count, int id) {
    for (int i = 0; i < count; i++) {
        if (i == id) continue;
        for (int j = 0; j < count; j++) {
            close(io[i].from[j]);
            close(io[i].to[j]);
        }
    }
}

void log_pipes(io_data *io) {
    /* Логгируем все пайпы*/
    for (int i = 0; i <= io->max_id; i++) {
        fprintf(io->pipes, "%i from %i = %i\n", io->current_id, i, io->from[i]);
    }
    for (int i = 0; i <= io->max_id; i++) {
        fprintf(io->pipes, "%i to %i = %i\n", io->current_id, i, io->to[i]);
    }
    fflush(io->pipes);
}

int child_loop(io_data io) {
    /* Первый этап */
    send_broadcast_and_wait_for_response(&io, STARTED);
    /* Второй этап, полезная работа */

    int done_count = 1;

    Message m;
    while (1) {
        int result = receive_any(&io, &m);
        if (result != 0) {
            sleep(0);
            continue;
        }

        if (m.s_header.s_type == STOP) {
            send_done_message(&io);
        } else if (m.s_header.s_type == TRANSFER) {
            TransferOrder *data = (TransferOrder *) m.s_payload;
            if (data->s_src == io.current_id) {

                fprintf(stdout, log_transfer_out_fmt, get_my_lamport_time(io.current_id), io.current_id, data->s_amount, data->s_dst);
                fflush(stdout);
                fprintf(io.events, log_transfer_out_fmt, get_my_lamport_time(io.current_id), io.current_id, data->s_amount,
                        data->s_dst);
                fflush(io.events);

                io.balance -= data->s_amount;

                assign_lamport_time(inc_lamport_time(io.current_id), m.s_header.s_local_timevector);
                send(&io, data->s_dst, &m);
            } else if (data->s_dst == io.current_id) {
                fprintf(stdout, log_transfer_in_fmt, get_my_lamport_time(io.current_id), io.current_id, data->s_amount, data->s_dst);
                fflush(stdout);
                fprintf(io.events, log_transfer_in_fmt, get_my_lamport_time(io.current_id), io.current_id, data->s_amount, data->s_dst);
                fflush(io.events);

                io.balance += data->s_amount;

                send_ack(&io, PARENT_ID);
            } else {
                fprintf(stderr, "Process %i get incorrect transfer message\n", io.current_id);
            }
        } else if (m.s_header.s_type == DONE) {
            done_count++;
            if (done_count >= io.max_id) {
                fprintf(stdout, log_received_all_done_fmt, get_my_lamport_time(io.current_id), io.current_id);
                fflush(stdout);
                fprintf(io.events, log_received_all_done_fmt, get_my_lamport_time(io.current_id), io.current_id);
                fflush(io.events);
                break;
            }
        }
    }

    //send_history(&io);

    return 0;
}

int main(int argc, char *argv[]) {
    int p = -1;

    /* Разбираем аргументы */
    int arg = getopt(argc, argv, "p:");
    while (arg != -1) {
        if (arg == 'p') {
            p = strtoimax(optarg, NULL, 10);
        }
        arg = getopt(argc, argv, "p:");
    }

    if (p == -1) {
        fprintf(stderr, "set processes count with -p\n");
        return 1;
    }

    /* Создаём io-структуры, которые раздадим детям */
    FILE *events = fopen(events_log, "w");
    FILE *pipes = fopen(pipes_log, "w");
    io_data io[p + 1];
    for (int i = 0; i <= p; i++) {
        io[i].current_id = i;
        io[i].max_id = p;

        io[i].events = events;
        io[i].pipes = pipes;

        if (i == 0) {
            io[i].balance = 0;
        } else {
            io[i].balance = strtoimax(argv[optind + i - 1], NULL, 10);
        }

        for (int j = 0; j <= p; j++) {
            if (i == j) {
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
    for (int i = 1; i <= p; i++) {
        int pid = fork();
        if (pid == 0) {
            /* Ребёнок */
            close_all_except(io, p + 1, i);
            return child_loop(io[i]);
        } else {
            /* Родитель */
        }
    }

    close_all_except(io, p + 1, PARENT_ID);

    wait_for_all(io, STARTED);

    bank_robbery(io, io[0].max_id);
    send_broadcast_stop(io);

    wait_for_all(io, DONE);
    return 0;
}
