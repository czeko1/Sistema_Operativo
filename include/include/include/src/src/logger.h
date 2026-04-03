#ifndef LOGGER_H
#define LOGGER_H

#include "../include/constants.h"

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_ERROR 3
#define LOG_LEVEL_CRITICAL 4

int init_logger(const char* filename);
void set_log_level(int level);
void log_message(int level, const char* format, ...);
void close_logger();

#endif // LOGGER_H
