#pragma once

//for pipe2
#define _GNU_SOURCE

#include <stdint.h>
#include <stdio.h>

#include "ipc.h"

#define MAX_PIPES 11

void create_pipe_matrix(int8_t num_processes, FILE *pipelog);

void close_spare_pipes(int8_t num_processes, local_id id);
void close_used_pipes(int8_t num_processes, local_id id);

void process_send_multicast(local_id id, int16_t type);
void process_receive_all(int8_t num_processes, local_id id, int16_t type);
void process_receive_any(local_id id);

void process_load();
