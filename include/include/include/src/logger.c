#include "logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>

static FILE* log_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static int log_level = LOG_LEVEL_INFO;

const char* level_strings[] = {
    "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"
};

int init_logger(const char* filename) {
    log_file = fopen(filename, "a");
    if (!log_file) {
        log_file = stderr;
        return ROUTER_ERROR_GENERAL;
    }
    return ROUTER_SUCCESS;
}

void set_log_level(int level) {
    log_level = level;
}

void log_message(int level, const char* format, ...) {
    if (level < log_level) return;
    
    pthread_mutex_lock(&log_mutex);
    
    time_t now = time(NULL);
    struct tm* local_time = localtime(&now);
    
    fprintf(log_file, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] ",
            local_time->tm_year + 1900,
            local_time->tm_mon + 1,
            local_time->tm_mday,
            local_time->tm_hour,
            local_time->tm_min,
            local_time->tm_sec,
            level_strings[level]);
    
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
    
    fprintf(log_file, "\n");
    fflush(log_file);
    
    pthread_mutex_unlock(&log_mutex);
}

void close_logger() {
    if (log_file && log_file != stderr) {
        fclose(log_file);
    }
    pthread_mutex_destroy(&log_mutex);
}
