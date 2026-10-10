/*
 * Log Queue Unit Test
 * Student ID: SE201072
 */

#include <stdio.h>
#include <string.h>

#include "logger_thread.h"

int main(void)
{
    LogQueue queue;
    char message[LOG_MESSAGE_SIZE];

    const char *expected[] = {
        "Sensor started",
        "Temperature=42.6",
        "High temperature alert"
    };

    if (log_queue_init(&queue) != 0) {
        printf("[TEST] FAIL: Queue initialization\n");
        return 1;
    }

    /* Add three messages */
    for (size_t i = 0; i < 3; i++) {
        if (log_queue_push(&queue, expected[i]) != 0) {
            printf("[TEST] FAIL: Push %zu\n", i);
            log_queue_shutdown(&queue);
            log_queue_destroy(&queue);
            return 1;
        }
    }

    /* Verify FIFO ordering */
    for (size_t i = 0; i < 3; i++) {
        if (log_queue_pop(&queue, message,
                          sizeof(message)) != 0) {
            printf("[TEST] FAIL: Pop %zu\n", i);
            log_queue_shutdown(&queue);
            log_queue_destroy(&queue);
            return 1;
        }

        printf("[LOG] %s\n", message);

        if (strcmp(message, expected[i]) != 0) {
            printf("[TEST] FAIL: FIFO mismatch\n");
            log_queue_shutdown(&queue);
            log_queue_destroy(&queue);
            return 1;
        }
    }

    log_queue_shutdown(&queue);

    /* Verify that shutdown rejects new messages */
    if (log_queue_push(&queue, "After shutdown") == 0) {
        printf("[TEST] FAIL: Push accepted after shutdown\n");
        log_queue_destroy(&queue);
        return 1;
    }

    log_queue_destroy(&queue);

    printf("[TEST] LOG QUEUE PASSED\n");

    return 0;
}
