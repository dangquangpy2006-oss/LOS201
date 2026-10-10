/*
 * Sensor Monitoring System - Logger Thread
 * Author: Nguyen Dang Quang
 * Student ID: SE201072
 *
 * Purpose:
 * Store log messages safely using a dedicated queue.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#include "logger_thread.h"
/*
 * Initialize the log queue.
 *
 * Return:
 *  0  = success
 * -1  = initialization failed
 */
int log_queue_init(LogQueue *queue)
{
    if (queue == NULL)
        return -1;

    memset(queue, 0, sizeof(*queue));

    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        return -1;
    }

    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        return -1;
    }

    return 0;
}
/*
 * Add one message to the log queue.
 *
 * Return:
 *  0  = success
 * -1  = queue full, shutting down, or invalid arguments
 */
int log_queue_push(LogQueue *queue, const char *message)
{
    if (queue == NULL || message == NULL)
        return -1;

    pthread_mutex_lock(&queue->mutex);

    /* Reject new messages during shutdown */
    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    /* Reject message if queue is full */
    if (queue->count >= LOG_QUEUE_CAPACITY) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    /* Copy message into the next free slot */
    snprintf(queue->messages[queue->tail],
             LOG_MESSAGE_SIZE,
             "%s",
             message);

    /* Advance tail with wraparound */
    queue->tail =
        (queue->tail + 1) % LOG_QUEUE_CAPACITY;

    queue->count++;

    /* Wake up Logger Thread */
    pthread_cond_signal(&queue->not_empty);

    pthread_mutex_unlock(&queue->mutex);

    return 0;
}
/*
 * Remove one message from the log queue.
 *
 * Return:
 *  0  = success
 * -1  = queue shutdown and empty, or invalid arguments
 */
int log_queue_pop(LogQueue *queue,
                  char *message,
                  size_t message_size)
{
    if (queue == NULL || message == NULL || message_size == 0)
        return -1;

    pthread_mutex_lock(&queue->mutex);

    /* Wait while the queue is empty */
    while (queue->count == 0 && !queue->shutdown) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }

    /* Stop when shutdown and no messages remain */
    if (queue->count == 0 && queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    /* Copy the oldest message */
    snprintf(message,
             message_size,
             "%s",
             queue->messages[queue->head]);

    /* Move head forward */
    queue->head =
        (queue->head + 1) % LOG_QUEUE_CAPACITY;

    queue->count--;

    pthread_mutex_unlock(&queue->mutex);

    return 0;
}
/*
 * Stop accepting new messages and wake up the Logger Thread.
 *
 * Messages already in the queue can still be processed.
 */

/*
 * Remove one message from the log queue with a timeout.
 *
 * Return:
 *  0  = message received
 *  1  = timeout
 * -1  = shutdown or error
 */
int log_queue_timedpop(LogQueue *queue,
                       char *message,
                       size_t message_size,
                       unsigned int timeout_ms)
{
    struct timespec deadline;
    int result;

    if (queue == NULL ||
        message == NULL ||
        message_size == 0) {
        return -1;
    }

    /* Calculate the absolute timeout */
    if (clock_gettime(CLOCK_REALTIME, &deadline) != 0) {
        return -1;
    }

    deadline.tv_sec += timeout_ms / 1000;

    deadline.tv_nsec +=
        (long)(timeout_ms % 1000) * 1000000L;

    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }

    pthread_mutex_lock(&queue->mutex);

    /* Wait until data arrives or timeout occurs */
    while (queue->count == 0 && !queue->shutdown) {

        result = pthread_cond_timedwait(
            &queue->not_empty,
            &queue->mutex,
            &deadline
        );

        if (result == ETIMEDOUT) {
            pthread_mutex_unlock(&queue->mutex);
            return 1;
        }

        if (result != 0) {
            pthread_mutex_unlock(&queue->mutex);
            return -1;
        }
    }

    /* Queue stopped and no messages remain */
    if (queue->count == 0 && queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    /* Copy oldest message */
    snprintf(message,
             message_size,
             "%s",
             queue->messages[queue->head]);

    /* Advance head */
    queue->head =
        (queue->head + 1) % LOG_QUEUE_CAPACITY;

    queue->count--;

    pthread_mutex_unlock(&queue->mutex);

    return 0;
}
void log_queue_shutdown(LogQueue *queue)
{
    if (queue == NULL)
        return;

    pthread_mutex_lock(&queue->mutex);

    /* Mark the queue as shutting down */
    queue->shutdown = 1;

    /* Wake up threads waiting for messages */
    pthread_cond_broadcast(&queue->not_empty);

    pthread_mutex_unlock(&queue->mutex);
}
/*
 * Release the resources used by the log queue.
 *
 * Call only after all threads using the queue have stopped.
 */
void log_queue_destroy(LogQueue *queue)
{
    if (queue == NULL)
        return;

    pthread_cond_destroy(&queue->not_empty);
    pthread_mutex_destroy(&queue->mutex);

    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    queue->shutdown = 1;
}
/*
 * Main function executed by the Logger Thread.
 *
 * Part 1: Open the log file.
 */
/*
 * Return the current size of a log file.
 *
 * Return:
 * >= 0 = file size in bytes
 * -1   = error
 */
static long get_log_file_size(FILE *log_file)
{
    long position;

    if (log_file == NULL)
        return -1;

    /* Ensure pending data reaches the file */
    if (fflush(log_file) != 0)
        return -1;

    /* Get current file position */
    position = ftell(log_file);

    return position;
}
/*
 * Rotate the log file when its size reaches the limit.
 *
 * Return:
 *  0 = success
 * -1 = error
 */
static int rotate_log_file(FILE **log_file,
                           const char *log_path)
{
    char rotated_path[512];
    FILE *new_file;
    int length;

    if (log_file == NULL ||
        *log_file == NULL ||
        log_path == NULL) {
        return -1;
    }

    length = snprintf(rotated_path,
                      sizeof(rotated_path),
                      "%s.1",
                      log_path);

    if (length < 0 ||
        (size_t)length >= sizeof(rotated_path)) {
        fprintf(stderr,
                "[LOGGER] Rotated path is too long\n");
        return -1;
    }

    /* Flush pending log data */
    if (fflush(*log_file) != 0) {
        perror("[LOGGER] fflush before rotation");
        return -1;
    }

    /* Close the current log file */
    if (fclose(*log_file) != 0) {
        perror("[LOGGER] fclose before rotation");
        *log_file = NULL;
        return -1;
    }

    *log_file = NULL;

    /* Rename the previous log file */
    if (rename(log_path, rotated_path) != 0) {
        perror("[LOGGER] rename");
        return -1;
    }

    /* Create a new active log file */
    new_file = fopen(log_path, "w");

    if (new_file == NULL) {
        perror("[LOGGER] fopen after rotation");
        return -1;
    }

    *log_file = new_file;

    printf("[LOGGER] Log rotated: %s\n",
           rotated_path);

    return 0;
}
void *logger_thread_main(void *arg)
{
    LoggerThreadConfig *config = (LoggerThreadConfig *)arg;
    const char *log_path;
    FILE *log_file;
    char message[LOG_MESSAGE_SIZE];
    unsigned long long written_count = 0;
    struct timespec last_flush;
    struct timespec now;
    int pop_result;
    long current_size;
    size_t message_length;
    if (config == NULL ||
        config->queue == NULL ||
        config->stop_requested == NULL) {
        return NULL;
    }

    log_path = config->log_path;

    if (log_path == NULL)
        log_path = "/tmp/sensor.log";

    log_file = fopen(log_path, "a");

    if (log_file == NULL) {
        perror("[LOGGER] fopen");
        return NULL;
    }

    printf("[LOGGER] Thread started\n");
    printf("[LOGGER] Log file: %s\n", log_path);
    /* Initialize periodic flush timer */
    if (clock_gettime(CLOCK_MONOTONIC, &last_flush) != 0) {
        perror("[LOGGER] clock_gettime");
        fclose(log_file);
        return NULL;
    }
      /*
     * Process messages and periodically flush log data.
     */
    while (1) {

        pop_result = log_queue_timedpop(
            config->queue,
            message,
            sizeof(message),
            200
        );

               if (pop_result == 0) {

            /* Determine the length of the new log entry */
            message_length = strlen(message) + 1;

            /* Check current log file size */
            current_size = get_log_file_size(log_file);

            if (current_size < 0) {
                fprintf(stderr,
                        "[LOGGER] Failed to get log file size\n");
                break;
            }

            /* Rotate before writing if limit would be exceeded */
            if ((unsigned long)current_size + message_length >
                LOG_MAX_FILE_SIZE) {

                if (rotate_log_file(&log_file, log_path) != 0) {
                    fprintf(stderr,
                            "[LOGGER] Log rotation failed\n");
                    break;
                }
            }

            /* Write the new log message */
            if (fprintf(log_file, "%s\n", message) < 0) {
                perror("[LOGGER] fprintf");
                break;
            }

            written_count++;
        }
        else if (pop_result == -1) {

            /* Shutdown and queue drained, or queue error */
            break;
        }

        /* Check elapsed time */
        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
            perror("[LOGGER] clock_gettime");
            break;
        }

        time_t elapsed_sec = now.tv_sec - last_flush.tv_sec;
        long elapsed_nsec = now.tv_nsec - last_flush.tv_nsec;

        if (elapsed_nsec < 0) {
            elapsed_sec--;
            elapsed_nsec += 1000000000L;
        }

        if (elapsed_sec >= LOG_FLUSH_INTERVAL_SEC) {

            if (fflush(log_file) != 0) {
                perror("[LOGGER] periodic fflush");
                break;
            }

            printf("[LOGGER] Periodic flush completed\n");

            last_flush = now;
        }
    }
if (log_file != NULL) {
    if (fflush(log_file) != 0) {
        perror("[LOGGER] fflush");
    }


    if (fclose(log_file) != 0) {
        perror("[LOGGER] fclose");
    }
  }
    printf("[LOGGER] Thread stopped. "
           "Total written: %llu\n",
           written_count);

    return NULL;
}
