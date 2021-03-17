#ifndef ARRAYQUEUE_H
#define ARRAYQUEUE_H



#include <pthread.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include "cacti.h"

/*
    +---+---+---+---+---+---+---+---+----+---+
    | 3 | 4 | 5 |(+)|   |   |   |   |1(-)| 2 |
    +---+---+---+---+---+---+---+---+----+---+
              ^                       ^
              |                       |
              |___tail                |___head
 */

/* dynamic queue of message_t */

typedef struct messages_queue {
    message_t *body;
    size_t size;
    size_t count;
    size_t head;
    size_t tail;
} messages_queue_t;

messages_queue_t *mq_new(size_t start_size);

message_t *mq_pop(messages_queue_t *q);

bool mq_push(messages_queue_t *q, message_t *msg);

bool mq_empty(messages_queue_t *q);

void mq_destroy(messages_queue_t *q);


/* dynamic queue of int */

struct int_queue;

typedef struct int_queue int_queue_t;

int_queue_t *iq_new(size_t start_size);

actor_id_t *iq_pop(int_queue_t *q);

bool iq_push(int_queue_t *q, actor_id_t *item);

bool iq_empty(int_queue_t *q);

void iq_destroy(int_queue_t *q);


#endif /* ARRAYQUEUE_H */