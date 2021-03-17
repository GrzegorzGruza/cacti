#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "arrayqueue.h"

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

typedef void (*thread_func_t)(void);

typedef struct tpool tpool_t;

tpool_t *tpool_create(size_t thread_num_,
                      thread_func_t worker_fun_);

void tpool_signal_new_work(tpool_t *tp);

void tpool_join(tpool_t *tp);

#endif /* THREADPOOL_H */