#ifndef ACTORS_H
#define ACTORS_H

#include "arrayqueue.h"
#include "cacti.h"

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

/* actor type */

typedef struct actor {
    messages_queue_t *msg_q;
    role_t *role;
    bool dead;
    bool ready_to_handling;
    void **stateptr;
    actor_id_t *my_id;
} actor_t;

actor_t *new_actor(role_t *role);

void destroy_actor(actor_t *act);

/* dynamic vector of actor_t */

typedef struct actors_vector {
    actor_t *body;
    actor_id_t count;
    actor_id_t size;
} actors_vector_t;

actors_vector_t *av_new(size_t start_size);

bool av_push_back(actors_vector_t *v, actor_t *act);

void av_destroy(actors_vector_t *av);

#endif /* ACTORS_H */