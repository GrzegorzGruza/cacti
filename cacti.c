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

#include "threadpool.h"
#include "arrayqueue.h"
#include "actors.h"

#define SUCCESS 0
#define FAILURE -1

#define ACTOR_IS_DEAD -1
#define ACTOR_NOT_EXISTS -2
#define QUEUE_IS_FULL -3
#define ALLOCATION_PROBLEM -4
#define SYSTEM_IS_DEAD -5


typedef struct {
    actors_vector_t *actors;
    int_queue_t *ready_actors;
    tpool_t *threads;
    pthread_mutex_t mutex;
    int number_of_totally_dead;
    pthread_cond_t wait_for_join;
    bool structure_memory_freed;
    pthread_key_t thread_actor;
    size_t number_of_waiting_for_join;
    pthread_cond_t wait_for_destroy_cacti;
    bool is_dead;
    pthread_t s_thread;
} catci_t;


catci_t main_cacti;

actor_id_t actor_id_self() {
    pthread_mutex_lock(&(main_cacti.mutex));
    actor_id_t my_id = (actor_id_t) pthread_getspecific(main_cacti.thread_actor);
    pthread_mutex_unlock(&(main_cacti.mutex));
    return my_id;
}

void destroy_cacti() {
    pthread_mutex_lock(&(main_cacti.mutex));
    if (main_cacti.structure_memory_freed) {
        pthread_mutex_unlock(&(main_cacti.mutex));
        return;
    }
    main_cacti.structure_memory_freed = true;
    if (main_cacti.actors != NULL) {
        av_destroy(main_cacti.actors);
        if (main_cacti.ready_actors != NULL) {
            iq_destroy(main_cacti.ready_actors);
            if (main_cacti.threads != NULL) {
                pthread_mutex_destroy(&(main_cacti.mutex));
                pthread_cond_destroy(&(main_cacti.wait_for_join));
                pthread_cond_destroy(&(main_cacti.wait_for_destroy_cacti));
                pthread_key_delete(main_cacti.thread_actor);
            }
        }
    }
    tpool_join(main_cacti.threads);
}


//signals
static void *sig_thread(void *arg) {

    sigset_t *set = arg;
    int s, sig;

    s = sigwait(set, &sig);
    if (s != 0) {
        exit(1);
    }
    pthread_mutex_lock(&(main_cacti.mutex));
    main_cacti.is_dead = true;
    for(int i = 0; i < main_cacti.actors->count; i++) {
        if(main_cacti.actors->body[i].msg_q->count == 0
        && !main_cacti.actors->body[i].dead) {
            ++main_cacti.number_of_totally_dead;
        }
    }
    if (main_cacti.number_of_totally_dead == main_cacti.actors->count) {
        pthread_cond_broadcast(&(main_cacti.wait_for_join));
        while (main_cacti.number_of_waiting_for_join > 0) {
            pthread_cond_wait(&(main_cacti.wait_for_destroy_cacti), &main_cacti.mutex);
        }
        if (!main_cacti.structure_memory_freed) {
            kill(getpid(), SIGINT);
        }
    } else {
        pthread_mutex_unlock(&(main_cacti.mutex));
    }

    pthread_mutex_unlock(&(main_cacti.mutex));

    return NULL;
}


void cacti_worker() {

    pthread_mutex_lock(&(main_cacti.mutex));

    actor_id_t actor_id = *iq_pop(main_cacti.ready_actors);

    message_t *msg = mq_pop(main_cacti.actors->body[actor_id].msg_q);
    actor_t *actor = &(main_cacti.actors->body[actor_id]);


    if (msg->message_type == MSG_GODIE) {
        actor->dead = true;
        ++main_cacti.number_of_totally_dead;
        pthread_mutex_unlock(&(main_cacti.mutex));
    } else if (msg->message_type == MSG_SPAWN) {
        actor_t *act_new = new_actor(msg->data);
        *(act_new->my_id) = main_cacti.actors->count;
        av_push_back(main_cacti.actors, act_new);
        pthread_setspecific(main_cacti.thread_actor, (void *) *main_cacti.actors->body[actor_id].my_id);

        message_t msg2;
        msg2.message_type = MSG_HELLO;
        pthread_mutex_unlock(&(main_cacti.mutex));
        actor_id_t myid = actor_id_self();
        pthread_mutex_lock(&(main_cacti.mutex));
        msg2.data = (void *) (myid);
        msg2.nbytes = sizeof(msg2.data);
        pthread_mutex_unlock(&(main_cacti.mutex));
        send_message(main_cacti.actors->count - 1, msg2);
        pthread_mutex_lock(&(main_cacti.mutex));

        pthread_mutex_unlock(&(main_cacti.mutex));
        free(act_new);
    } else {
        pthread_mutex_unlock(&(main_cacti.mutex));
        pthread_setspecific(main_cacti.thread_actor, (void *) *main_cacti.actors->body[actor_id].my_id);
        actor->role->prompts[msg->message_type]((void **) &actor->stateptr, msg->nbytes, msg->data);
    }
    pthread_mutex_lock(&(main_cacti.mutex));
    if (main_cacti.actors->body[actor_id].msg_q->count == 0) {
        main_cacti.actors->body[actor_id].ready_to_handling = false;
        if (main_cacti.is_dead) {
            ++main_cacti.number_of_totally_dead;
        }
    } else {
        iq_push((main_cacti.ready_actors), &actor_id);
        tpool_signal_new_work(main_cacti.threads);
    }
    if (main_cacti.number_of_totally_dead == main_cacti.actors->count) {
        pthread_cond_broadcast(&(main_cacti.wait_for_join));
        while (main_cacti.number_of_waiting_for_join > 0) {
            pthread_cond_wait(&(main_cacti.wait_for_destroy_cacti), &main_cacti.mutex);
        }
        if (!main_cacti.structure_memory_freed) {
            pthread_mutex_unlock(&(main_cacti.mutex));
            destroy_cacti();
        }
    } else {
        pthread_mutex_unlock(&(main_cacti.mutex));
    }
}


int send_message(actor_id_t actor, message_t message) {

    pthread_mutex_lock(&(main_cacti.mutex));
    if (main_cacti.actors->count <= actor) {
        pthread_mutex_unlock(&(main_cacti.mutex));
        return ACTOR_NOT_EXISTS;
    }
    if (main_cacti.is_dead) {
        return SYSTEM_IS_DEAD;
    }
    if (main_cacti.actors->body[actor].dead) {
        pthread_mutex_unlock(&(main_cacti.mutex));
        return ACTOR_IS_DEAD;
    }
    if (main_cacti.actors->body[actor].msg_q->count == ACTOR_QUEUE_LIMIT) {
        pthread_mutex_unlock(&(main_cacti.mutex));
        return QUEUE_IS_FULL;
    }
    if (!mq_push(main_cacti.actors->body[actor].msg_q, &message)) {
        pthread_mutex_unlock(&(main_cacti.mutex));
        return ALLOCATION_PROBLEM;
    }
    if (!main_cacti.actors->body[actor].ready_to_handling) {
        if (iq_push((main_cacti.ready_actors), &actor)) {
            main_cacti.actors->body[actor].ready_to_handling = true;
            tpool_signal_new_work(main_cacti.threads);
        } else {
            pthread_mutex_unlock(&(main_cacti.mutex));
            return ALLOCATION_PROBLEM;
        }
    }
    pthread_mutex_unlock(&(main_cacti.mutex));
    return SUCCESS;
}

int actor_system_create(actor_id_t *actor, role_t *const role) {
    main_cacti.structure_memory_freed = false;
    main_cacti.actors = av_new(1024);
    if (main_cacti.actors == NULL) {
        destroy_cacti();
        return FAILURE;
    }
    main_cacti.ready_actors = iq_new(1024);
    if (main_cacti.ready_actors == NULL) {
        destroy_cacti();
        return FAILURE;
    }

    // signals
    sigset_t set;
    int s;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    s = pthread_sigmask(SIG_BLOCK, &set, NULL);
    if (s != 0) {
        exit(1);
    }
    s = pthread_create(&main_cacti.s_thread, NULL, &sig_thread, &set);
    pthread_detach(main_cacti.s_thread);
    if (s != 0) {
        exit(1);
    }

    main_cacti.threads = tpool_create(POOL_SIZE, cacti_worker);
    if (main_cacti.threads == NULL) {
        destroy_cacti();
        return FAILURE;
    }


    actor_t *act = new_actor(role);
    *(act->my_id) = 0;
    if (act == NULL) {
        destroy_cacti();
        return FAILURE;
    }
    if (!av_push_back(main_cacti.actors, act)) {
        free(act);
        destroy_cacti();
        return FAILURE;
    }

    *actor = 0;
    role->prompts[MSG_HELLO]((void **) &(main_cacti.actors->body[0].stateptr), 0, NULL);

    free(act);

    pthread_mutex_init(&(main_cacti.mutex), 0);
    main_cacti.number_of_totally_dead = 0;
    pthread_key_create(&main_cacti.thread_actor, NULL);
    pthread_cond_init(&(main_cacti.wait_for_join), 0);
    pthread_cond_init(&(main_cacti.wait_for_destroy_cacti), 0);
    main_cacti.number_of_waiting_for_join = 0;
    main_cacti.is_dead = false;
    return SUCCESS;
}

void actor_system_join(actor_id_t actor) {
    pthread_mutex_lock(&(main_cacti.mutex));
    if (main_cacti.actors->count <= actor) {
        pthread_mutex_unlock(&(main_cacti.mutex));
        return;
    }
    ++main_cacti.number_of_waiting_for_join;
    while (main_cacti.number_of_totally_dead != main_cacti.actors->count) {
        pthread_cond_wait(&(main_cacti.wait_for_join), &(main_cacti.mutex));
    }
    --main_cacti.number_of_waiting_for_join;
    pthread_mutex_unlock(&(main_cacti.mutex));
    pthread_cond_signal(&(main_cacti.wait_for_destroy_cacti));
}

