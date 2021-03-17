#include "arrayqueue.h"

/* dynamic queue of message_t */

messages_queue_t *mq_new(size_t start_size) {
    messages_queue_t *q = malloc(sizeof(messages_queue_t));
    if (q == NULL) {
        return NULL;
    }
    q->body = malloc(sizeof(message_t) * start_size);
    if (q->body == NULL) {
        free(q);
        return NULL;
    }
    q->size = start_size;
    q->count = 0;
    q->head = 0;
    q->tail = -1;
    return q;
}

message_t *mq_pop(messages_queue_t *q) {
    if (q->count == 0) {
        return NULL;
    }
    message_t *to_return = q->body + q->head;
    q->head = (q->head + 1) % q->size;
    --q->count;
    return to_return;
}

bool mq_increase_twice(messages_queue_t *q) {
    message_t *new_body = malloc(sizeof(message_t) * q->size * 2);
    if (new_body == NULL) {
        return false;
    }
    size_t i = 0;
    for(; i + q->head < q->size; ++i){
        new_body[i] = q->body[i + q->head];
    }
    for(size_t j = 0; j <= q->tail; ++j){
        new_body[i+j] = q->body[j];
    }
    q->size *= 2;
    q->head = 0;
    q->tail = q->count - 1;
    free(q->body);
    q->body = new_body;
    return true;
}

bool mq_push(messages_queue_t *q, message_t *msg) {
    if (q->size == q->count) {
        if(!mq_increase_twice(q)) {
            return false;
        }
    }
    q->tail = (q->tail + 1) % q->size;
    ++q->count;
    q->body[q->tail] = *msg;
    return true;
}

bool mq_empty(messages_queue_t *q) {
    return q->count == 0;
}

void mq_destroy(messages_queue_t *q){
    free(q->body);
    free(q);
}


/* dynamic queue of int */

typedef struct int_queue {
    actor_id_t *body;
    size_t size;
    size_t count;
    size_t head;
    size_t tail;
} int_queue_t;

int_queue_t *iq_new(size_t start_size) {
    int_queue_t *q = malloc(sizeof(int_queue_t));
    if (q == NULL) {
        return NULL;
    }
    q->body = malloc(sizeof(actor_id_t) * start_size);
    if (q->body == NULL) {
        free(q);
        return NULL;
    }
    q->size = start_size;
    q->count = 0;
    q->head = 0;
    q->tail = -1;
    return q;
}

actor_id_t *iq_pop(int_queue_t *q) {
    if (q->count == 0) {
        return NULL;
    }
    actor_id_t *to_return = q->body + q->head;
    q->head = (q->head + 1) % q->size;
    --q->count;
    return to_return;
}

bool iq_increase_twice(int_queue_t *q) {
    actor_id_t *new_body = malloc(sizeof(actor_id_t) * q->size * 2);
    if (new_body == NULL) {
        return false;
    }
    size_t i = 0;
    for(; i + q->head < q->size; ++i){
        new_body[i] = q->body[i + q->head];
    }
    for(size_t j = 0; j <= q->tail; ++j){
        new_body[i+j] = q->body[j];
    }
    q->size *= 2;
    q->head = 0;
    q->tail = q->count - 1;
    free(q->body);
    q->body = new_body;
    return true;
}

bool iq_push(int_queue_t *q, actor_id_t *item) {
    if (q->size == q->count) {
        if(!iq_increase_twice(q)) {
            return false;
        }
    }
    q->tail = (q->tail + 1) % q->size;
    ++q->count;
    q->body[q->tail] = *item;
    return true;
}

bool iq_empty(int_queue_t *q) {
    return q->count == 0;
}

void iq_destroy(int_queue_t *q) {
    free(q->body);
    free(q);
}