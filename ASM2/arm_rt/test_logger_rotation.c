/*
 * Sensor Monitoring System
 * Log Rotation Test
 * Student ID: SE201072
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>

#include "logger_thread.h"

int main(void)
{
    LogQueue queue;
    LoggerThreadConfig config;
    pthread_t thread;

    atomic_bool stop_requested = ATOMIC_VAR_INIT(false);

    const char *log_path = "/tmp/sms_rotation_test.log";
    const char *rotated_path = "/tmp/sms_rotation_test.log.1";
    const char *message = "Rotation test message";

    FILE *file;
    struct stat active_stat;
    struct stat rotated_stat;
    char line[LOG_MESSAGE_SIZE];

    /* Remove previous test files */
    remove(log_path);
    remove(rotated_path);

    /*
     * Create a log file just below 1 MiB.
     */
    file = fopen(log_path, "wb");

    if (file == NULL) {
        perror("[TEST] fopen");
        return 1;
    }

    const size_t initial_size = LOG_MAX_FILE_SIZE - 10;

    for (size_t i = 0; i < initial_size; i++) {
        if (fputc('A', file) == EOF) {
            perror("[TEST] fputc");
            fclose(file);
            return 1;
        }
    }

    if (fclose(file) != 0) {
        perror("[TEST] fclose");
        return 1;
    }

    printf("[TEST] Initial log size: %zu bytes\n",
           initial_size);

    if (log_queue_init(&queue) != 0) {
        fprintf(stderr, "[TEST] Queue init failed\n");
        return 1;
    }

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
        fprintf(stderr, "[TEST] Push failed\n");

        atomic_store(&stop_requested, true);
        log_queue_shutdown(&queue);
        pthread_join(thread, NULL);
        log_queue_destroy(&queue);

        return 1;
    }

    /* Let Logger Thread drain the queued message */
    atomic_store(&stop_requested, true);
    log_queue_shutdown(&queue);

    pthread_join(thread, NULL);

    log_queue_destroy(&queue);

    /* Check both files */
    if (stat(log_path, &active_stat) != 0) {
        perror("[TEST] stat active log");
        return 1;
    }

    if (stat(rotated_path, &rotated_stat) != 0) {
        perror("[TEST] stat rotated log");
        return 1;
    }

    printf("[TEST] Active log size: %lld bytes\n",
           (long long)active_stat.st_size);

    printf("[TEST] Rotated log size: %lld bytes\n",
           (long long)rotated_stat.st_size);

    if ((size_t)rotated_stat.st_size != initial_size) {
        fprintf(stderr,
                "[TEST] Rotated file size mismatch\n");
        return 1;
    }

    if (active_stat.st_size <= 0 ||
        (size_t)active_stat.st_size > LOG_MAX_FILE_SIZE) {
        fprintf(stderr,
                "[TEST] Invalid active file size\n");
        return 1;
    }

    /* Check the new log contains the expected message */
    file = fopen(log_path, "r");

    if (file == NULL) {
        perror("[TEST] fopen active log");
        return 1;
    }

    if (fgets(line, sizeof(line), file) == NULL) {
        fprintf(stderr, "[TEST] Active log is empty\n");
        fclose(file);
        return 1;
    }

    fclose(file);

    line[strcspn(line, "\n")] = '\0';

    if (strcmp(line, message) != 0) {
        fprintf(stderr,
                "[TEST] Active log content mismatch\n");
        return 1;
    }

    printf("[TEST] LOG ROTATION PASSED\n");

    return 0;
}

