#include <sys/mman.h>
/*
 * Sensor Monitoring System - Main Application
 *
 * Student: Nguyen Dang Quang
 * Student ID: SE201072
 *
 * Assignment 02 - Embedded Linux
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "sensor_data.h"
#include "circular_buffer.h"
#include "sensor_thread.h"
#include "processor_thread.h"
#include "alert_thread.h"
#include "logger_thread.h"

/* Device and POSIX message queue settings */
#define SENSOR_DEVICE_PATH "/dev/sms_sensor"
#define ALERT_QUEUE_NAME "/sms_alert_queue"

/* Sensor sampling interval */
#define SENSOR_INTERVAL_MS 500

/* Global stop flag shared between threads */
static atomic_bool stop_requested = ATOMIC_VAR_INIT(false);

/* SIGINT handler: request graceful shutdown */

static volatile sig_atomic_t sigint_seen = 0;

static void handle_sigint(int signo)
{
    (void)signo;
    sigint_seen = 1;
}


int main(void)
{
    CircularBuffer sensor_buffer;
    LogQueue log_queue;
    struct mq_attr alert_attr;
    mqd_t alert_queue = (mqd_t)-1;

    pthread_t sensor_tid;
    pthread_t processor_tid;
    pthread_t alert_tid;
    pthread_t logger_tid;

    bool sensor_started = false;
    bool processor_started = false;
    bool alert_started = false;
    bool logger_started = false;

    atomic_bool alert_stop = ATOMIC_VAR_INIT(false);
    atomic_bool logger_stop = ATOMIC_VAR_INIT(false);

    SensorThreadConfig sensor_config;
    ProcessorThreadConfig processor_config;
    AlertThreadConfig alert_config;
    LoggerThreadConfig logger_config;

    struct sigaction sa;
    struct timespec delay = {0, 200000000L};
    int result;
    int exit_status = EXIT_SUCCESS;

    printf("====================================\n");
    printf("      SENSOR MONITORING SYSTEM\n");
    printf("====================================\n");
    printf("[MAIN] Student ID: SE201072\n");

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) != 0) {
        perror("[MAIN] sigaction");
        return EXIT_FAILURE;
    }

    if (circular_buffer_init(&sensor_buffer, 64) != 0) {
        fprintf(stderr, "[MAIN] Circular Buffer init failed\n");
        return EXIT_FAILURE;
    }

    memset(&alert_attr, 0, sizeof(alert_attr));
    alert_attr.mq_maxmsg = 10;
    alert_attr.mq_msgsize = sizeof(AlertMessage);

    if (mq_unlink(ALERT_QUEUE_NAME) != 0 && errno != ENOENT) {
        perror("[MAIN] mq_unlink");
        circular_buffer_destroy(&sensor_buffer);
        return EXIT_FAILURE;
    }

    alert_queue = mq_open(
        ALERT_QUEUE_NAME,
        O_CREAT | O_EXCL | O_RDWR,
        0600,
        &alert_attr
    );

    if (alert_queue == (mqd_t)-1) {
        perror("[MAIN] mq_open");
        circular_buffer_destroy(&sensor_buffer);
        return EXIT_FAILURE;
    }

    if (log_queue_init(&log_queue) != 0) {
        fprintf(stderr, "[MAIN] Log Queue init failed\n");
        mq_close(alert_queue);
        mq_unlink(ALERT_QUEUE_NAME);
        circular_buffer_destroy(&sensor_buffer);
        return EXIT_FAILURE;
    }

    printf("[MAIN] Circular Buffer initialized (64)\n");
    printf("[MAIN] POSIX Message Queue initialized\n");
    printf("[MAIN] Log Queue initialized (128)\n");

    sensor_config.buffer = &sensor_buffer;
    sensor_config.interval_ms = SENSOR_INTERVAL_MS;
    sensor_config.device_path = SENSOR_DEVICE_PATH;
    sensor_config.stop_requested = &stop_requested;
    sensor_config.log_queue = &log_queue;

    processor_config.buffer = &sensor_buffer;
    processor_config.alert_queue = alert_queue;
    processor_config.stop_requested = &stop_requested;
    processor_config.log_queue = &log_queue;

    alert_config.alert_queue = alert_queue;
    alert_config.stop_requested = &alert_stop;
    alert_config.log_queue = &log_queue;

    logger_config.queue = &log_queue;
    logger_config.log_path = "/tmp/sensor.log";
    logger_config.stop_requested = &logger_stop;

    /*
     * Start consumers before producers.
     */
    /* Prevent paging delays during real-time operation. */
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        perror("[RT] mlockall");
        fprintf(stderr,
                "[RT] Warning: memory locking unavailable\n");
    } else {
        printf("[RT] mlockall enabled\n");
    }

    result = pthread_create(
        &logger_tid, NULL, logger_thread_main, &logger_config
    );

    if (result != 0) {
        fprintf(stderr, "[MAIN] Logger create: %s\n",
                strerror(result));
        exit_status = EXIT_FAILURE;
        goto shutdown;
    }
    logger_started = true;

    result = pthread_create(
        &alert_tid, NULL, alert_thread_main, &alert_config
    );

    if (result != 0) {
        fprintf(stderr, "[MAIN] Alert create: %s\n",
                strerror(result));
        exit_status = EXIT_FAILURE;
        goto shutdown;
    }
    alert_started = true;

    result = pthread_create(
        &processor_tid, NULL, processor_thread_main,
        &processor_config
    );

    if (result != 0) {
        fprintf(stderr, "[MAIN] Processor create: %s\n",
                strerror(result));
        exit_status = EXIT_FAILURE;
        goto shutdown;
    }
    processor_started = true;

    result = pthread_create(
        &sensor_tid, NULL, sensor_thread_main, &sensor_config
    );

    if (result != 0) {
        fprintf(stderr, "[MAIN] Sensor create: %s\n",
                strerror(result));
        exit_status = EXIT_FAILURE;
        goto shutdown;
    }
    sensor_started = true;

    printf("[MAIN] All 4 POSIX Threads started\n");

    if (log_queue_push(&log_queue,
                       "SMS application started") != 0) {
        fprintf(stderr, "[MAIN] Startup log queue full\n");
    }

    printf("[MAIN] Press Ctrl+C to stop\n");

    while (!sigint_seen) {
        nanosleep(&delay, NULL);
    }

shutdown:
    printf("\n[MAIN] Shutdown requested\n");

    /*
     * 1. Stop sensor producer first.
     */
    atomic_store(&stop_requested, true);

    if (sensor_started) {
        pthread_join(sensor_tid, NULL);
        printf("[MAIN] Sensor Thread joined\n");
    }

    /*
     * 2. Wake the processor, then let it drain samples.
     */
    circular_buffer_shutdown(&sensor_buffer);

    if (processor_started) {
        pthread_join(processor_tid, NULL);
        printf("[MAIN] Processor Thread joined\n");
    }

    /*
     * 3. No more alerts will be produced.
     */
    atomic_store(&alert_stop, true);

    if (alert_started) {
        pthread_join(alert_tid, NULL);
        printf("[MAIN] Alert Thread joined\n");
    }

    /*
     * 4. Stop logging after other workers finish.
     */
    if (log_queue_push(&log_queue,
                       "SMS application shutting down") != 0) {
        fprintf(stderr, "[MAIN] Shutdown log queue full\n");
    }

    atomic_store(&logger_stop, true);
    log_queue_shutdown(&log_queue);

    if (logger_started) {
        pthread_join(logger_tid, NULL);
        printf("[MAIN] Logger Thread joined\n");
    }

    log_queue_destroy(&log_queue);

    if (mq_close(alert_queue) != 0) {
        perror("[MAIN] mq_close");
        exit_status = EXIT_FAILURE;
    }

    if (mq_unlink(ALERT_QUEUE_NAME) != 0) {
        perror("[MAIN] mq_unlink");
        exit_status = EXIT_FAILURE;
    }

    circular_buffer_destroy(&sensor_buffer);

    printf("[MAIN] Resources cleaned up\n");
    printf("[MAIN] Application stopped\n");

    return exit_status;
}
