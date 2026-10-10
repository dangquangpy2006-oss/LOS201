/*
 * POSIX Message Queue Test
 * Student ID: SE201072
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <mqueue.h>
#include <sys/stat.h>

#include "sensor_data.h"

int main(void)
{
    const char *queue_name = "/sms_mqueue_test";

    struct mq_attr attr = {0};
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(AlertMessage);

    mq_unlink(queue_name);

    mqd_t queue = mq_open(queue_name,
                          O_CREAT | O_RDWR,
                          0600,
                          &attr);

    if (queue == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }

    AlertMessage sent = {
        .timestamp_ms = 12345,
        .type = TEMP_HIGH,
        .value = 42.6f,
        .threshold = 40.0f
    };

    if (mq_send(queue,
                (const char *)&sent,
                sizeof(sent),
                0) == -1) {
        perror("mq_send");
        mq_close(queue);
        mq_unlink(queue_name);
        return 1;
    }

    AlertMessage received;

    if (mq_receive(queue,
                   (char *)&received,
                   sizeof(received),
                   NULL) == -1) {
        perror("mq_receive");
        mq_close(queue);
        mq_unlink(queue_name);
        return 1;
    }

    printf("[MQUEUE] Message received\n");
    printf("Timestamp: %llu\n",
           (unsigned long long)received.timestamp_ms);
    printf("Type: %d\n", received.type);
    printf("Value: %.1f\n", received.value);
    printf("Threshold: %.1f\n", received.threshold);

    mq_close(queue);
    mq_unlink(queue_name);

    printf("[MQUEUE] TEST PASSED\n");

    return 0;
}
