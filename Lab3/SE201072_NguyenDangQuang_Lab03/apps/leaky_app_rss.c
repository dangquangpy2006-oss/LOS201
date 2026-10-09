#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NUM_RECORDS 100
#define MSG_SIZE 256

typedef struct {
    int id;
    char *name;
    int *scores;
} Record;

/* Leak 1: Cap phat bo nho cho record */
Record *create_record(int id, const char *name, int num_scores) {
    Record *r = malloc(sizeof(Record));

    r->id = id;
    r->name = malloc(strlen(name) + 1);
    strcpy(r->name, name);

    r->scores = malloc(sizeof(int) * num_scores);

    for (int i = 0; i < num_scores; i++)
        r->scores[i] = id * 10 + i;

    return r;
}

/* Bug: Quen giai phong name va scores */
void delete_record(Record *r) {
    /* free(r->name); */
    /* free(r->scores); */
    free(r);
}

/* Leak 2: Quen giai phong buffer */
void process_messages(int count) {
    for (int i = 0; i < count; i++) {
        char *buf = malloc(MSG_SIZE);

        snprintf(buf, MSG_SIZE,
                 "Message #%d: data payload here", i);

        if (i % 10 == 0)
            printf(" Processed: %s\n", buf);

        /* free(buf); */
    }
}


void print_memory_status(const char *stage) {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) {
        perror("Cannot open /proc/self/status");
        return;
    }

    char line[256];

    printf("\n========== %s ==========\n", stage);

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmSize:", 7) == 0 ||
            strncmp(line, "VmRSS:", 6) == 0 ||
            strncmp(line, "VmData:", 7) == 0) {
            printf("%s", line);
        }
    }

    fclose(f);
}

int main(void) {
    printf("=== Leaky App - LAB-03 Memory Analysis ===\n");

    printf("[1] Creating %d records...\n", NUM_RECORDS);

    print_memory_status("BEFORE ALLOCATION");

    Record **records = malloc(sizeof(Record*) * NUM_RECORDS);

    for (int i = 0; i < NUM_RECORDS; i++)
        records[i] = create_record(i, "EmbeddedLinuxStudent", 5);

    printf("[2] Deleting records (with leak)...\n");

    for (int i = 0; i < NUM_RECORDS; i++)
        delete_record(records[i]);

    free(records);

    printf("[3] Processing messages (with leak)...\n");

    process_messages(50);
    print_memory_status("AFTER ALLOCATION AND PROCESSING");
    sleep(30);

    printf("Program finished - check valgrind output!\n");

    return 0;
}
