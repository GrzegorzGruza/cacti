#include "cacti.h"
#include <unistd.h>
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

#define MSG_RESPOND 1
#define MSG_FACTORIAL 2
unsigned long long n;

typedef struct {
    unsigned long long k;
    unsigned long long fact;
} factorial_t;

role_t role_normal;
role_t role_first;

void hello_execute_first(void **stateptr __attribute__((unused)),
                         size_t nbytes __attribute__((unused)),
                         void *data __attribute__((unused))) {
}

void hello_execute(void **stateptr __attribute__((unused)),
                   size_t nbytes __attribute__((unused)),
                   void *data) {
    actor_id_t parent = ((actor_id_t) data);
    message_t msg;
    msg.message_type = MSG_RESPOND;
    actor_id_t myid = actor_id_self();
    msg.data = (void *) (myid);
    msg.nbytes = sizeof(msg.data);
    send_message(parent, msg);
}

void respond_execute(void **stateptr,
                     size_t nbytes __attribute__((unused)),
                     void *data) {
    actor_id_t child = (actor_id_t) data;
    message_t msg;
    msg.message_type = MSG_FACTORIAL;
    msg.data = (*stateptr);
    msg.nbytes = sizeof(msg.data);

    send_message(child, msg);

    msg.message_type = MSG_GODIE;
    send_message(actor_id_self(), msg);
}

void factorial_execute(void **stateptr,
                       size_t nbytes __attribute__((unused)),
                       void *data) {
    *stateptr = data;
    factorial_t *my_factorial = (factorial_t *) (*stateptr);
    ++my_factorial->k;
    my_factorial->fact *= my_factorial->k;

    if (my_factorial->k == n) {
        printf("%llu", my_factorial->fact);
        message_t msg;
        msg.message_type = MSG_GODIE;
        msg.data = NULL;
        msg.nbytes = sizeof(msg.data);
        send_message(actor_id_self(), msg);
        return;
    }

    message_t msg;
    msg.message_type = MSG_SPAWN;
    msg.data = (void *) (&role_normal);
    msg.nbytes = sizeof(msg.data);

    send_message(actor_id_self(), msg);
}

static act_t prompts_normal[3] = {hello_execute, respond_execute, factorial_execute};
static act_t prompts_first[3] = {hello_execute_first, respond_execute, factorial_execute};

int main() {
    actor_id_t first_actor;
    role_normal.nprompts = 3;
    role_normal.prompts = prompts_normal;
    role_first.nprompts = 3;
    role_first.prompts = prompts_first;

    scanf("%llu", &n);

    actor_system_create(&first_actor, &role_first);

    factorial_t factor;
    factor.k = 0;
    factor.fact = 1;

    message_t msg;
    msg.message_type = MSG_FACTORIAL;
    msg.data = (void *) (&factor);
    msg.nbytes = sizeof(msg.data);
    send_message(first_actor, msg);

    actor_system_join(first_actor);

    return 0;
}
