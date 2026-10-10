/*
 * Sensor Monitoring System - Sensor Thread
 * Author: Nguyen Dang Quang
 * Student ID: SE201072
 * Date: 10 October 2026
 */

#ifndef SENSOR_THREAD_H
#define SENSOR_THREAD_H

#include <stdatomic.h>
#include "logger_thread.h"

#include "circular_buffer.h"

typedef struct {
    CircularBuffer *buffer;

    unsigned int interval_ms;
    const char *device_path;

    atomic_bool *stop_requested;
    LogQueue *log_queue;
} SensorThreadConfig;

/* Main function executed by the Sensor Thread */
void *sensor_thread_main(void *arg);

#endif

