/*
 * Logger Thread Integration Test
 * Student ID: SE201072
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include <string.h>

#include "logger_thread.h"

int main(void)
{
    LogQueue queue;
    LoggerThreadConfig config;
    pthread_t thread;

    atomic_bool stop_requested = ATOMIC_VAR_INIT(false);

    const char *log_path = "/tmp/sms_logger_test.log";

    const char *messages[] = {
        "Sensor Thread started",
        "Temperature=42.6, Humidity=85.0",
        "High temperature alert detected"
    };

    if (log_queue_init(&queue) != 0) {
        fprintf(stderr, "[TEST] Queue initialization failed\n");
        return 1;
    }

    /* Remove the previous test log */
    remove(log_path);

    config.queue = &queue;
    config.log_path = log_path;
    config.stop_requested = &stop_requested;

    int result = pthread_create(
        &thread,
        NULL,
        logger_thread_main,
        &config
    );

    if (result != 0) {
        fprintf(stderr, "[TEST] pthread_create failed\n");
        log_queue_destroy(&queue);
        return 1;
    }

    for (size_t i = 0; i < 3; i++) {
        if (log_queue_push(&queue, messages[i]) != 0) {
            fprintf(stderr, "[TEST] Failed to push message\n");
        }
    }

    /* Process remaining messages before stopping */
    atomic_store(&stop_requested, true);
    log_queue_shutdown(&queue);

    pthread_join(thread, NULL);

    log_queue_destroy(&queue);

    /* Verify the generated log file */
    FILE *file = fopen(log_path, "r");

    if (file == NULL) {
        perror("[TEST] fopen");
        return 1;
    }

    char line[LOG_MESSAGE_SIZE];
    int line_count = 0;
    int valid = 1;

    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\n")] = '\0';

        printf("[LOG FILE] %s\n", line);

        if (line_count >= 3 ||
            strcmp(line, messages[line_count]) != 0) {
            valid = 0;
        }

        line_count++;
    }

    fclose(file);

    if (valid && line_count == 3) {
        printf("[TEST] LOGGER THREAD PASSED\n");
        return 0;
    }

    printf("[TEST] LOGGER THREAD FAILED\n");
    return 1;
}
