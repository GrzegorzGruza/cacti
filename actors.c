#include "actors.h"

/* actor type */

actor_t *new_actor(role_t *role) {
    actor_t *actor = malloc(sizeof(actor_t));
    if (actor == NULL) {
        return NULL;
    }
    messages_queue_t *q = mq_new(2);
    if(q == NULL) {
        free(actor);
        return NULL;
    }
    actor_id_t *_my_id = malloc(sizeof(actor_id_t));
    if(_my_id == NULL) {
        free(actor);
        free(q);
        return NULL;
    }
    actor->msg_q = q;
    actor->role = role;
    actor->dead = false;
    actor->ready_to_handling = false;
    actor->stateptr = NULL;
    actor->my_id = _my_id;
    return actor;
}

void destroy_actor(actor_t *act) {
    if(act->msg_q != NULL) {
        mq_destroy(act->msg_q);
    }
    if(act->my_id != NULL) {
        free(act->my_id);
    }
}

/* dynamic vector of actor_t */

actors_vector_t *av_new(size_t start_size) {
    actors_vector_t *v = malloc(sizeof(actor_t));
    if(v == NULL) {
        return NULL;
    }
    v->body = malloc(sizeof(actor_t) * start_size);
    if(v->body == NULL) {
        free(v);
        return NULL;
    }
    v->count = 0;
    v->size = start_size;
    return v;
}

bool av_push_back(actors_vector_t *v, actor_t *act) {
    if (v->size == v->count) {
        v->size *= 2;
        v->body = realloc(v->body, v->size);
        if(v->body == NULL) {
            return false;
        }
    }
    v->body[v->count++] = *act;
    return true;
}

void av_destroy(actors_vector_t *av) {
    if(av == NULL) {
        return;
    }
    for(actor_id_t i = 0; i < av->count; ++i) {
        destroy_actor(av->body + i);
    }
    free(av->body);
}