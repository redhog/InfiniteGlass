#ifndef CONCURRENCY_H
#define CONCURRENCY_H

#define WITH(setup, teardown) for (int _with_code = ({ setup; 0; }); _with_code < 1; ({ teardown; _with_code++; }))

#define WITH_LOCK(lock) WITH(pthread_mutex_lock(lock), pthread_mutex_unlock(lock))


#endif

