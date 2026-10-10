/*
 * Alert Thread Integration Test
 * Student ID: SE201072
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#include "alert_thread.h"
#include "sensor_data.h"

int main(void)
{
    const char *queue_name = "/sms_alert_test";

    pthread_t thread;
    AlertThreadConfig config;

    atomic_bool stop_requested = ATOMIC_VAR_INIT(false);

    struct mq_attr attr = {0};
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(AlertMessage);

    mq_unlink(queue_name);

    /*
     * Open in blocking mode so mq_timedreceive()
     * can wait for messages efficiently.
     */
    mqd_t queue = mq_open(
        queue_name,
        O_CREAT | O_RDWR,
        0600,
        &attr
    );

    if (queue == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }

    config.alert_queue = queue;
    config.stop_requested = &stop_requested;

    if (pthread_create(&thread, NULL,
                       alert_thread_main, &config) != 0) {
        fprintf(stderr, "pthread_create failed\n");
        mq_close(queue);
        mq_unlink(queue_name);
        return 1;
    }

    printf("[TEST] Alert Thread created\n");

    AlertMessage alerts[] = {
        {1000, TEMP_HIGH, 42.6f, 40.0f},
        {2000, HUMID_HIGH, 85.0f, 80.0f},
        {3000, TEMP_SPIKE, 7.0f, 5.0f}
    };

    for (size_t i = 0; i < 3; i++) {
        if (mq_send(queue,
                    (const char *)&alerts[i],
                    sizeof(AlertMessage),
                    0) == -1) {
            perror("mq_send");
            break;
        }
    }

    /*
     * Tell Alert Thread to stop after processing
     * the messages already in the queue.
     */
    atomic_store(&stop_requested, true);

    pthread_join(thread, NULL);

    mq_close(queue);
    mq_unlink(queue_name);

    printf("[TEST] Alert Thread finished\n");

    return 0;
}
