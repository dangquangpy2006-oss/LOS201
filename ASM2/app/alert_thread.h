/*
 * Sensor Monitoring System - Alert Thread
 * Student ID: SE201072
 */

#ifndef ALERT_THREAD_H
#define ALERT_THREAD_H

#include <stdatomic.h>
#include "logger_thread.h"
#include <mqueue.h>

#include "sensor_data.h"

/*
 * Configuration for the Alert Thread.
 */
typedef struct {
    mqd_t alert_queue;

    atomic_bool *stop_requested;
    LogQueue *log_queue;
} AlertThreadConfig;

/* Main function executed by the Alert Thread */
void *alert_thread_main(void *arg);

#endif
