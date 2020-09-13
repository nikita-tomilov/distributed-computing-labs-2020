#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

extern FILE *eventlog;

void fail_gracefully(char* fail_string) {
    fclose(eventlog);
    perror(fail_string);
    exit(EXIT_FAILURE);
}

void fail_custom(char* fail_string) {
    fclose(eventlog);
    fprintf(stderr, "%s\n", fail_string);
    exit(EXIT_FAILURE);
}

void llog(FILE* file, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    
    va_start(args, fmt);
    vfprintf(file, fmt, args);
    va_end(args);
}
