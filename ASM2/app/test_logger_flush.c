/*
 * Sensor Monitoring System
 * Periodic Flush Test
 * Student ID: SE201072
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#include "logger_thread.h"

int main(void)
{
    LogQueue queue;
    LoggerThreadConfig config;
    pthread_t thread;

    atomic_bool stop_requested = ATOMIC_VAR_INIT(false);

    const char *log_path = "/tmp/sms_flush_test.log";
    const char *message = "Periodic flush test message";

    if (log_queue_init(&queue) != 0) {
        fprintf(stderr, "[TEST] Queue initialization failed\n");
        return 1;
    }

    remove(log_path);

    config.queue = &queue;
    config.log_path = log_path;
    config.stop_requested = &stop_requested;

    if (pthread_create(&thread, NULL,
                       logger_thread_main, &config) != 0) {
        fprintf(stderr, "[TEST] Thread creation failed\n");
        log_queue_destroy(&queue);
        return 1;
    }

    if (log_queue_push(&queue, message) != 0) {
        fprintf(stderr, "[TEST] Failed to push log message\n");

        atomic_store(&stop_requested, true);
        log_queue_shutdown(&queue);
        pthread_join(thread, NULL);
        log_queue_destroy(&queue);

        return 1;
    }

    printf("[TEST] Waiting 12 seconds...\n");
    fflush(stdout);

    /* Keep Logger Thread alive for 12 seconds */
    struct timespec delay = {
        .tv_sec = 12,
        .tv_nsec = 0
    };

    while (nanosleep(&delay, &delay) != 0) {
        /* Retry if interrupted */
    }

    atomic_store(&stop_requested, true);
    log_queue_shutdown(&queue);

    pthread_join(thread, NULL);
    log_queue_destroy(&queue);

    /* Verify final log file content */
    FILE *file = fopen(log_path, "r");

    if (file == NULL) {
        perror("[TEST] fopen");
        return 1;
    }

    char line[LOG_MESSAGE_SIZE];

    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        fprintf(stderr, "[TEST] Log file is empty\n");
        return 1;
    }

    fclose(file);

    line[strcspn(line, "\n")] = '\0';

    if (strcmp(line, message) != 0) {
        fprintf(stderr, "[TEST] Log content mismatch\n");
        return 1;
    }

    printf("[TEST] Log content verified\n");
    printf("[TEST] PERIODIC FLUSH PASSED\n");

    return 0;
}
