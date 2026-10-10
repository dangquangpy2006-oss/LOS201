/*
 * Sensor Monitoring System - Processor Thread
 * Student ID: SE201072
 */

#ifndef PROCESSOR_THREAD_H
#define PROCESSOR_THREAD_H

#include <stdatomic.h>
#include "logger_thread.h"
#include <mqueue.h>

#include "circular_buffer.h"
#include "sensor_data.h"

#define TEMP_ALERT_THRESHOLD 40.0f
#define HUMID_ALERT_THRESHOLD 80.0f
#define TEMP_SPIKE_THRESHOLD 5.0f

typedef struct {
    CircularBuffer *buffer;

    mqd_t alert_queue;

    atomic_bool *stop_requested;
    LogQueue *log_queue;
} ProcessorThreadConfig;

/* Main function executed by Processor Thread */
void *processor_thread_main(void *arg);

#endif
