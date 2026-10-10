/*
 * Sensor Monitoring System - Logger Thread
 * Student ID: SE201072
 */

#ifndef LOGGER_THREAD_H
#define LOGGER_THREAD_H

#include <stdatomic.h>
#include <pthread.h>
#include <stddef.h>

#define LOG_QUEUE_CAPACITY 128
#define LOG_MESSAGE_SIZE 256

#define LOG_FLUSH_INTERVAL_SEC 5
#define LOG_MAX_FILE_SIZE (1024 * 1024)

typedef struct {
    char messages[LOG_QUEUE_CAPACITY][LOG_MESSAGE_SIZE];

    size_t head;
    size_t tail;
    size_t count;

    int shutdown;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
} LogQueue;

typedef struct {
    LogQueue *queue;

    const char *log_path;

    atomic_bool *stop_requested;
} LoggerThreadConfig;

/* Log queue operations */
int log_queue_init(LogQueue *queue);
int log_queue_push(LogQueue *queue, const char *message);
int log_queue_pop(LogQueue *queue, char *message,
                  size_t message_size);
/*
 * Pop a message with a timeout.
 *
 * Return:
 *  0 = success
 *  1 = timeout
 * -1 = shutdown or error
 */
int log_queue_timedpop(LogQueue *queue,
                       char *message,
                       size_t message_size,
                       unsigned int timeout_ms);
void log_queue_shutdown(LogQueue *queue);
void log_queue_destroy(LogQueue *queue);

/* Main function executed by Logger Thread */
void *logger_thread_main(void *arg);

#endif
