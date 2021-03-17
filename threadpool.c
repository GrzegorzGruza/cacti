#include "threadpool.h"

typedef struct tpool {
    thread_func_t worker_fun;
    pthread_cond_t wait_for_work;
    pthread_cond_t wait_for_destroy;
    pthread_mutex_t mutex;
    size_t thread_num;
    bool destroyed;
    size_t number_of_work;
    pthread_t *threads_arr;
} tpool_t;


void *worker(void *arg) {
    tpool_t *tp = (tpool_t *) arg;
    while (true) {
        pthread_mutex_lock(&(tp->mutex));
        while (tp->number_of_work == 0 && !tp->destroyed) {
            pthread_cond_wait(&(tp->wait_for_work), &(tp->mutex));
        }

        if (tp->number_of_work == 0 && tp->destroyed) {
            break;
        }
        --tp->number_of_work;
        pthread_mutex_unlock(&(tp->mutex));
        tp->worker_fun();
    }
    --tp->thread_num;
    if (tp->thread_num == 0) {
        pthread_cond_signal(&(tp->wait_for_destroy));
        pthread_mutex_unlock(&(tp->mutex));
    } else {
        pthread_mutex_unlock(&(tp->mutex));
    }
    return NULL;
}

tpool_t *tpool_create(size_t thread_num_,
                      thread_func_t worker_fun_) {

    tpool_t *tp = malloc(sizeof(tpool_t));
    if (tp == NULL) {
        return NULL;
    }
    tp->threads_arr = malloc(sizeof(pthread_t) * thread_num_);
    if (tp->threads_arr == NULL) {
        free(tp);
        return NULL;
    }

    tp->worker_fun = worker_fun_;
    pthread_cond_init(&(tp->wait_for_work), NULL);
    pthread_cond_init(&(tp->wait_for_destroy), NULL);
    pthread_mutex_init(&(tp->mutex), NULL);
    tp->destroyed = false;
    tp->thread_num = thread_num_;
    tp->number_of_work = 0;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    for (size_t i = 0; i < thread_num_; i++) {
        pthread_create(&tp->threads_arr[i], &attr, worker, tp);
    }

    return tp;
}

void tpool_signal_new_work(tpool_t *tp) {
    if (tp->destroyed) {
        return;
    }
    pthread_mutex_lock(&(tp->mutex));
    ++tp->number_of_work;
    pthread_mutex_unlock(&(tp->mutex));
    pthread_cond_signal(&(tp->wait_for_work));
}

void tpool_join(tpool_t *tp) {
    pthread_mutex_lock(&(tp->mutex));
    tp->destroyed = true;
    pthread_cond_broadcast(&(tp->wait_for_work));
    while (tp->thread_num > 0) {
        pthread_cond_wait(&(tp->wait_for_destroy), &(tp->mutex));
    }

    void *retval;
    for (size_t i = 0; i < tp->thread_num; i++) {
        pthread_join(tp->threads_arr[i], &retval);
    }

    free(tp->threads_arr);
    pthread_mutex_unlock(&(tp->mutex));
    pthread_mutex_destroy(&(tp->mutex));
    pthread_cond_destroy(&(tp->wait_for_work));
    pthread_cond_destroy(&(tp->wait_for_destroy));
    free(tp);
}