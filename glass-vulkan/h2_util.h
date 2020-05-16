/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _h2_util_
#define _h2_util_

#define ap_assert assert

#include <assert.h>
#include <apr_strings.h>
#include <apr_thread_mutex.h>
#include <apr_thread_cond.h>
#include <apr_hash.h>
#include <apr_buckets.h>

/*
#include <httpd.h>
#include <http_core.h>
#include <http_log.h>
#include <http_request.h>

#include <nghttp2/nghttp2.h>

#include "h2.h"
#include "h2_util.h"
*/


/*******************************************************************************
 * ihash - hash for structs with int identifier
 ******************************************************************************/
typedef struct h2_ihash_t h2_ihash_t;
typedef int h2_ihash_iter_t(void *ctx, void *val);

/**
 * Create a hash for structures that have an identifying int member.
 * @param pool the pool to use
 * @param offset_of_int the offsetof() the int member in the struct
 */
h2_ihash_t *h2_ihash_create(apr_pool_t *pool, size_t offset_of_int);

size_t h2_ihash_count(h2_ihash_t *ih);
int h2_ihash_empty(h2_ihash_t *ih);
void *h2_ihash_get(h2_ihash_t *ih, int id);

/**
 * Iterate over the hash members (without defined order) and invoke
 * fn for each member until 0 is returned.
 * @param ih the hash to iterate over
 * @param fn the function to invoke on each member
 * @param ctx user supplied data passed into each iteration call
 * @return 0 if one iteration returned 0, otherwise != 0
 */
int h2_ihash_iter(h2_ihash_t *ih, h2_ihash_iter_t *fn, void *ctx);

void h2_ihash_add(h2_ihash_t *ih, void *val);
void h2_ihash_remove(h2_ihash_t *ih, int id);
void h2_ihash_remove_val(h2_ihash_t *ih, void *val);
void h2_ihash_clear(h2_ihash_t *ih);

size_t h2_ihash_shift(h2_ihash_t *ih, void **buffer, size_t max);

/*******************************************************************************
 * iqueue - sorted list of int with user defined ordering
 ******************************************************************************/
typedef struct h2_iqueue {
    int *elts;
    int head;
    int nelts;
    int nalloc;
    apr_pool_t *pool;
} h2_iqueue;

/**
 * Comparator for two int to determine their order.
 *
 * @param i1 first int to compare
 * @param i2 second int to compare
 * @param ctx provided user data
 * @return value is the same as for strcmp() and has the effect:
 *    == 0: s1 and s2 are treated equal in ordering
 *     < 0: s1 should be sorted before s2
 *     > 0: s2 should be sorted before s1
 */
typedef int h2_iq_cmp(int i1, int i2, void *ctx);

/**
 * Allocate a new queue from the pool and initialize.
 * @param pool the memory pool
 * @param capacity the initial capacity of the queue
 */
h2_iqueue *h2_iq_create(apr_pool_t *pool, int capacity);

/**
 * Return != 0 iff there are no tasks in the queue.
 * @param q the queue to check
 */
int h2_iq_empty(h2_iqueue *q);

/**
 * Return the number of int in the queue.
 * @param q the queue to get size on
 */
int h2_iq_count(h2_iqueue *q);

/**
 * Add a stream id to the queue. 
 *
 * @param q the queue to append the id to
 * @param sid the stream id to add
 * @param cmp the comparator for sorting
 * @param ctx user data for comparator
 * @return != 0 iff id was not already there 
 */
int h2_iq_add(h2_iqueue *q, int sid, h2_iq_cmp *cmp, void *ctx);

/**
 * Append the id to the queue if not already present. 
 *
 * @param q the queue to append the id to
 * @param sid the id to append
 * @return != 0 iff id was not already there 
 */
int h2_iq_append(h2_iqueue *q, int sid);

/**
 * Remove the stream id from the queue. Return != 0 iff task
 * was found in queue.
 * @param q the task queue
 * @param sid the stream id to remove
 * @return != 0 iff task was found in queue
 */
int h2_iq_remove(h2_iqueue *q, int sid);

/**
 * Remove all entries in the queue.
 */
void h2_iq_clear(h2_iqueue *q);

/**
 * Sort the stream idqueue again. Call if the task ordering
 * has changed.
 *
 * @param q the queue to sort
 * @param cmp the comparator for sorting
 * @param ctx user data for the comparator 
 */
void h2_iq_sort(h2_iqueue *q, h2_iq_cmp *cmp, void *ctx);

/**
 * Get the first id from the queue or 0 if the queue is empty. 
 * The id is being removed.
 *
 * @param q the queue to get the first id from
 * @return the first id of the queue, 0 if empty
 */
int h2_iq_shift(h2_iqueue *q);

/**
 * Get the first max ids from the queue. All these ids will be removed.
 *
 * @param q the queue to get the first task from
 * @param pint the int array to receive the values
 * @param max the maximum number of ids to shift
 * @return the actual number of ids shifted
 */
size_t h2_iq_mshift(h2_iqueue *q, int *pint, size_t max);

/**
 * Determine if int is in the queue already
 *
 * @param q the queue
 * @param sid the integer id to check for
 * @return != 0 iff sid is already in the queue
 */
int h2_iq_contains(h2_iqueue *q, int sid);

/*******************************************************************************
 * FIFO queue (void* elements)
 ******************************************************************************/

/**
 * A thread-safe FIFO queue with some extra bells and whistles, if you
 * do not need anything special, better use 'apr_queue'.
 */
typedef struct h2_fifo h2_fifo;

/**
 * Create a FIFO queue that can hold up to capacity elements. Elements can
 * appear several times.
 */
apr_status_t h2_fifo_create(h2_fifo **pfifo, apr_pool_t *pool, int capacity);

/**
 * Create a FIFO set that can hold up to capacity elements. Elements only
 * appear once. Pushing an element already present does not change the
 * queue and is successful.
 */
apr_status_t h2_fifo_set_create(h2_fifo **pfifo, apr_pool_t *pool, int capacity);

apr_status_t h2_fifo_term(h2_fifo *fifo);

int h2_fifo_count(h2_fifo *fifo);

/**
 * Push en element into the queue. Blocks if there is no capacity left.
 * 
 * @param fifo the FIFO queue
 * @param elem the element to push
 * @return APR_SUCCESS on push, APR_EAGAIN on try_push on a full queue,
 *         APR_EEXIST when in set mode and elem already there.
 */
apr_status_t h2_fifo_push(h2_fifo *fifo, void *elem);
apr_status_t h2_fifo_try_push(h2_fifo *fifo, void *elem);

apr_status_t h2_fifo_pull(h2_fifo *fifo, void **pelem);
apr_status_t h2_fifo_try_pull(h2_fifo *fifo, void **pelem);

typedef enum {
    H2_FIFO_OP_PULL,   /* pull the element from the queue, ie discard it */
    H2_FIFO_OP_REPUSH, /* pull and immediately re-push it */
} h2_fifo_op_t;

typedef h2_fifo_op_t h2_fifo_peek_fn(void *head, void *ctx);

/**
 * Call given function on the head of the queue, once it exists, and
 * perform the returned operation on it. The queue will hold its lock during
 * this time, so no other operations on the queue are possible.
 * @param fifo the queue to peek at
 * @param fn   the function to call on the head, once available
 * @param ctx  context to pass in call to function
 */
apr_status_t h2_fifo_peek(h2_fifo *fifo, h2_fifo_peek_fn *fn, void *ctx);

/**
 * Non-blocking version of h2_fifo_peek.
 */
apr_status_t h2_fifo_try_peek(h2_fifo *fifo, h2_fifo_peek_fn *fn, void *ctx);

/**
 * Remove the elem from the queue, will remove multiple appearances.
 * @param elem  the element to remove
 * @return APR_SUCCESS iff > 0 elems were removed, APR_EAGAIN otherwise.
 */
apr_status_t h2_fifo_remove(h2_fifo *fifo, void *elem);

/*******************************************************************************
 * iFIFO queue (int elements)
 ******************************************************************************/

/**
 * A thread-safe FIFO queue with some extra bells and whistles, if you
 * do not need anything special, better use 'apr_queue'.
 */
typedef struct h2_ififo h2_ififo;

/**
 * Create a FIFO queue that can hold up to capacity int. ints can
 * appear several times.
 */
apr_status_t h2_ififo_create(h2_ififo **pfifo, apr_pool_t *pool, int capacity);

/**
 * Create a FIFO set that can hold up to capacity integers. Ints only
 * appear once. Pushing an int already present does not change the
 * queue and is successful.
 */
apr_status_t h2_ififo_set_create(h2_ififo **pfifo, apr_pool_t *pool, int capacity);

apr_status_t h2_ififo_term(h2_ififo *fifo);

int h2_ififo_count(h2_ififo *fifo);

/**
 * Push an int into the queue. Blocks if there is no capacity left.
 * 
 * @param fifo the FIFO queue
 * @param id  the int to push
 * @return APR_SUCCESS on push, APR_EAGAIN on try_push on a full queue,
 *         APR_EEXIST when in set mode and elem already there.
 */
apr_status_t h2_ififo_push(h2_ififo *fifo, int id);
apr_status_t h2_ififo_try_push(h2_ififo *fifo, int id);

apr_status_t h2_ififo_pull(h2_ififo *fifo, int *pi);
apr_status_t h2_ififo_try_pull(h2_ififo *fifo, int *pi);

typedef h2_fifo_op_t h2_ififo_peek_fn(int head, void *ctx);

/**
 * Call given function on the head of the queue, once it exists, and
 * perform the returned operation on it. The queue will hold its lock during
 * this time, so no other operations on the queue are possible.
 * @param fifo the queue to peek at
 * @param fn   the function to call on the head, once available
 * @param ctx  context to pass in call to function
 */
apr_status_t h2_ififo_peek(h2_ififo *fifo, h2_ififo_peek_fn *fn, void *ctx);

/**
 * Non-blocking version of h2_fifo_peek.
 */
apr_status_t h2_ififo_try_peek(h2_ififo *fifo, h2_ififo_peek_fn *fn, void *ctx);

/**
 * Remove the integer from the queue, will remove multiple appearances.
 * @param id  the integer to remove
 * @return APR_SUCCESS iff > 0 ints were removed, APR_EAGAIN otherwise.
 */
apr_status_t h2_ififo_remove(h2_ififo *fifo, int id);

/*******************************************************************************
 * common helpers
 ******************************************************************************/

/**
 * Count the bytes that all key/value pairs in a table have
 * in length (exlucding terminating 0s), plus additional extra per pair.
 *
 * @param t the table to inspect
 * @param pair_extra the extra amount to add per pair
 * @return the number of bytes all key/value pairs have
 */
apr_size_t h2_util_table_bytes(apr_table_t *t, apr_size_t pair_extra);

/** Match a header value against a string constance, case insensitive */
#define H2_HD_MATCH_LIT(l, name, nlen)  \
    ((nlen == sizeof(l) - 1) && !apr_strnatcasecmp(l, name))

#ifdef H2_UTIL_IMPLEMENTATION

/*******************************************************************************
 * ihash - hash for structs with int identifier
 ******************************************************************************/
struct h2_ihash_t {
    apr_hash_t *hash;
    size_t ioff;
};

static unsigned int ihash(const char *key, apr_ssize_t *klen)
{
    return (unsigned int)(*((int*)key));
}

h2_ihash_t *h2_ihash_create(apr_pool_t *pool, size_t offset_of_int)
{
    h2_ihash_t *ih = apr_pcalloc(pool, sizeof(h2_ihash_t));
    ih->hash = apr_hash_make_custom(pool, ihash);
    ih->ioff = offset_of_int;
    return ih;
}

size_t h2_ihash_count(h2_ihash_t *ih)
{
    return apr_hash_count(ih->hash);
}

int h2_ihash_empty(h2_ihash_t *ih)
{
    return apr_hash_count(ih->hash) == 0;
}

void *h2_ihash_get(h2_ihash_t *ih, int id)
{
    return apr_hash_get(ih->hash, &id, sizeof(id));
}

typedef struct {
    h2_ihash_iter_t *iter;
    void *ctx;
} iter_ctx;

static int ihash_iter(void *ctx, const void *key, apr_ssize_t klen, 
                     const void *val)
{
    iter_ctx *ictx = ctx;
    return ictx->iter(ictx->ctx, (void*)val); /* why is this passed const?*/
}

int h2_ihash_iter(h2_ihash_t *ih, h2_ihash_iter_t *fn, void *ctx)
{
    iter_ctx ictx;
    ictx.iter = fn;
    ictx.ctx = ctx;
    return apr_hash_do(ihash_iter, &ictx, ih->hash);
}

void h2_ihash_add(h2_ihash_t *ih, void *val)
{
    apr_hash_set(ih->hash, ((char *)val + ih->ioff), sizeof(int), val);
}

void h2_ihash_remove(h2_ihash_t *ih, int id)
{
    apr_hash_set(ih->hash, &id, sizeof(id), NULL);
}

void h2_ihash_remove_val(h2_ihash_t *ih, void *val)
{
    int id = *((int*)((char *)val + ih->ioff));
    apr_hash_set(ih->hash, &id, sizeof(id), NULL);
}


void h2_ihash_clear(h2_ihash_t *ih)
{
    apr_hash_clear(ih->hash);
}

typedef struct {
    h2_ihash_t *ih;
    void **buffer;
    size_t max;
    size_t len;
} collect_ctx;

static int collect_iter(void *x, void *val)
{
    collect_ctx *ctx = x;
    if (ctx->len < ctx->max) {
        ctx->buffer[ctx->len++] = val;
        return 1;
    }
    return 0;
}

size_t h2_ihash_shift(h2_ihash_t *ih, void **buffer, size_t max)
{
    collect_ctx ctx;
    size_t i;
    
    ctx.ih = ih;
    ctx.buffer = buffer;
    ctx.max = max;
    ctx.len = 0;
    h2_ihash_iter(ih, collect_iter, &ctx);
    for (i = 0; i < ctx.len; ++i) {
        h2_ihash_remove_val(ih, buffer[i]);
    }
    return ctx.len;
}

/*******************************************************************************
 * iqueue - sorted list of int
 ******************************************************************************/

static void iq_grow(h2_iqueue *q, int nlen);
static void iq_swap(h2_iqueue *q, int i, int j);
static int iq_bubble_up(h2_iqueue *q, int i, int top, 
                        h2_iq_cmp *cmp, void *ctx);
static int iq_bubble_down(h2_iqueue *q, int i, int bottom, 
                          h2_iq_cmp *cmp, void *ctx);

h2_iqueue *h2_iq_create(apr_pool_t *pool, int capacity)
{
    h2_iqueue *q = apr_pcalloc(pool, sizeof(h2_iqueue));
    if (q) {
        q->pool = pool;
        iq_grow(q, capacity);
        q->nelts = 0;
    }
    return q;
}

int h2_iq_empty(h2_iqueue *q)
{
    return q->nelts == 0;
}

int h2_iq_count(h2_iqueue *q)
{
    return q->nelts;
}


int h2_iq_add(h2_iqueue *q, int sid, h2_iq_cmp *cmp, void *ctx)
{
    int i;
    
    if (h2_iq_contains(q, sid)) {
        return 0;
    }
    if (q->nelts >= q->nalloc) {
        iq_grow(q, q->nalloc * 2);
    }
    i = (q->head + q->nelts) % q->nalloc;
    q->elts[i] = sid;
    ++q->nelts;
    
    if (cmp) {
        /* bubble it to the front of the queue */
        iq_bubble_up(q, i, q->head, cmp, ctx);
    }
    return 1;
}

int h2_iq_append(h2_iqueue *q, int sid)
{
    return h2_iq_add(q, sid, NULL, NULL);
}

int h2_iq_remove(h2_iqueue *q, int sid)
{
    int i;
    for (i = 0; i < q->nelts; ++i) {
        if (sid == q->elts[(q->head + i) % q->nalloc]) {
            break;
        }
    }
    
    if (i < q->nelts) {
        ++i;
        for (; i < q->nelts; ++i) {
            q->elts[(q->head+i-1)%q->nalloc] = q->elts[(q->head+i)%q->nalloc];
        }
        --q->nelts;
        return 1;
    }
    return 0;
}

void h2_iq_clear(h2_iqueue *q)
{
    q->nelts = 0;
}

void h2_iq_sort(h2_iqueue *q, h2_iq_cmp *cmp, void *ctx)
{
    /* Assume that changes in ordering are minimal. This needs,
     * best case, q->nelts - 1 comparisons to check that nothing
     * changed.
     */
    if (q->nelts > 0) {
        int i, ni, prev, last;
        
        /* Start at the end of the queue and create a tail of sorted
         * entries. Make that tail one element longer in each iteration.
         */
        last = i = (q->head + q->nelts - 1) % q->nalloc;
        while (i != q->head) {
            prev = (q->nalloc + i - 1) % q->nalloc;
            
            ni = iq_bubble_up(q, i, prev, cmp, ctx);
            if (ni == prev) {
                /* i bubbled one up, bubble the new i down, which
                 * keeps all tasks below i sorted. */
                iq_bubble_down(q, i, last, cmp, ctx);
            }
            i = prev;
        };
    }
}


int h2_iq_shift(h2_iqueue *q)
{
    int sid;
    
    if (q->nelts <= 0) {
        return 0;
    }
    
    sid = q->elts[q->head];
    q->head = (q->head + 1) % q->nalloc;
    q->nelts--;
    
    return sid;
}

size_t h2_iq_mshift(h2_iqueue *q, int *pint, size_t max)
{
    int i;
    for (i = 0; i < max; ++i) {
        pint[i] = h2_iq_shift(q);
        if (pint[i] == 0) {
            break;
        }
    }
    return i;
}

static void iq_grow(h2_iqueue *q, int nlen)
{
    if (nlen > q->nalloc) {
        int *nq = apr_pcalloc(q->pool, sizeof(int) * nlen);
        if (q->nelts > 0) {
            int l = ((q->head + q->nelts) % q->nalloc) - q->head;
            
            memmove(nq, q->elts + q->head, sizeof(int) * l);
            if (l < q->nelts) {
                /* elts wrapped, append elts in [0, remain] to nq */
                int remain = q->nelts - l;
                memmove(nq + l, q->elts, sizeof(int) * remain);
            }
        }
        q->elts = nq;
        q->nalloc = nlen;
        q->head = 0;
    }
}

static void iq_swap(h2_iqueue *q, int i, int j)
{
    int x = q->elts[i];
    q->elts[i] = q->elts[j];
    q->elts[j] = x;
}

static int iq_bubble_up(h2_iqueue *q, int i, int top, 
                        h2_iq_cmp *cmp, void *ctx) 
{
    int prev;
    while (((prev = (q->nalloc + i - 1) % q->nalloc), i != top) 
           && (*cmp)(q->elts[i], q->elts[prev], ctx) < 0) {
        iq_swap(q, prev, i);
        i = prev;
    }
    return i;
}

static int iq_bubble_down(h2_iqueue *q, int i, int bottom, 
                          h2_iq_cmp *cmp, void *ctx)
{
    int next;
    while (((next = (q->nalloc + i + 1) % q->nalloc), i != bottom) 
           && (*cmp)(q->elts[i], q->elts[next], ctx) > 0) {
        iq_swap(q, next, i);
        i = next;
    }
    return i;
}

int h2_iq_contains(h2_iqueue *q, int sid)
{
    int i;
    for (i = 0; i < q->nelts; ++i) {
        if (sid == q->elts[(q->head + i) % q->nalloc]) {
            return 1;
        }
    }
    return 0;
}

/*******************************************************************************
 * FIFO queue
 ******************************************************************************/

struct h2_fifo {
    void **elems;
    int nelems;
    int set;
    int head;
    int count;
    int aborted;
    apr_thread_mutex_t *lock;
    apr_thread_cond_t  *not_empty;
    apr_thread_cond_t  *not_full;
};

static int nth_index(h2_fifo *fifo, int n) 
{
    return (fifo->head + n) % fifo->nelems;
}

static apr_status_t fifo_destroy(void *data) 
{
    h2_fifo *fifo = data;

    apr_thread_cond_destroy(fifo->not_empty);
    apr_thread_cond_destroy(fifo->not_full);
    apr_thread_mutex_destroy(fifo->lock);

    return APR_SUCCESS;
}

static int index_of(h2_fifo *fifo, void *elem)
{
    int i;
    
    for (i = 0; i < fifo->count; ++i) {
        if (elem == fifo->elems[nth_index(fifo, i)]) {
            return i;
        }
    }
    return -1;
}

static apr_status_t create_int(h2_fifo **pfifo, apr_pool_t *pool, 
                               int capacity, int as_set)
{
    apr_status_t rv;
    h2_fifo *fifo;
    
    fifo = apr_pcalloc(pool, sizeof(*fifo));
    if (fifo == NULL) {
        return APR_ENOMEM;
    }

    rv = apr_thread_mutex_create(&fifo->lock,
                                 APR_THREAD_MUTEX_UNNESTED, pool);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    rv = apr_thread_cond_create(&fifo->not_empty, pool);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    rv = apr_thread_cond_create(&fifo->not_full, pool);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    fifo->elems = apr_pcalloc(pool, capacity * sizeof(void*));
    if (fifo->elems == NULL) {
        return APR_ENOMEM;
    }
    fifo->nelems = capacity;
    fifo->set = as_set;
    
    *pfifo = fifo;
    apr_pool_cleanup_register(pool, fifo, fifo_destroy, apr_pool_cleanup_null);

    return APR_SUCCESS;
}

apr_status_t h2_fifo_create(h2_fifo **pfifo, apr_pool_t *pool, int capacity)
{
    return create_int(pfifo, pool, capacity, 0);
}

apr_status_t h2_fifo_set_create(h2_fifo **pfifo, apr_pool_t *pool, int capacity)
{
    return create_int(pfifo, pool, capacity, 1);
}

apr_status_t h2_fifo_term(h2_fifo *fifo)
{
    apr_status_t rv;
    if ((rv = apr_thread_mutex_lock(fifo->lock)) == APR_SUCCESS) {
        fifo->aborted = 1;
        apr_thread_cond_broadcast(fifo->not_empty);
        apr_thread_cond_broadcast(fifo->not_full);
        apr_thread_mutex_unlock(fifo->lock);
    }
    return rv;
}

int h2_fifo_count(h2_fifo *fifo)
{
    return fifo->count;
}

static apr_status_t check_not_empty(h2_fifo *fifo, int block)
{
    while (fifo->count == 0) {
        if (!block) {
            return APR_EAGAIN;
        }
        if (fifo->aborted) {
            return APR_EOF;
        }
        apr_thread_cond_wait(fifo->not_empty, fifo->lock);
    }
    return APR_SUCCESS;
}

static apr_status_t fifo_push_int(h2_fifo *fifo, void *elem, int block)
{
    if (fifo->aborted) {
        return APR_EOF;
    }

    if (fifo->set && index_of(fifo, elem) >= 0) {
        /* set mode, elem already member */
        return APR_EEXIST;
    }
    else if (fifo->count == fifo->nelems) {
        if (block) {
            while (fifo->count == fifo->nelems) {
                if (fifo->aborted) {
                    return APR_EOF;
                }
                apr_thread_cond_wait(fifo->not_full, fifo->lock);
            }
        }
        else {
            return APR_EAGAIN;
        }
    }
    
    ap_assert(fifo->count < fifo->nelems);
    fifo->elems[nth_index(fifo, fifo->count)] = elem;
    ++fifo->count;
    if (fifo->count == 1) {
        apr_thread_cond_broadcast(fifo->not_empty);
    }
    return APR_SUCCESS;
}

static apr_status_t fifo_push(h2_fifo *fifo, void *elem, int block)
{
    apr_status_t rv;
    
    if ((rv = apr_thread_mutex_lock(fifo->lock)) == APR_SUCCESS) {
        rv = fifo_push_int(fifo, elem, block);
        apr_thread_mutex_unlock(fifo->lock);
    }
    return rv;
}

apr_status_t h2_fifo_push(h2_fifo *fifo, void *elem)
{
    return fifo_push(fifo, elem, 1);
}

apr_status_t h2_fifo_try_push(h2_fifo *fifo, void *elem)
{
    return fifo_push(fifo, elem, 0);
}

static apr_status_t pull_head(h2_fifo *fifo, void **pelem, int block)
{
    apr_status_t rv;
    
    if ((rv = check_not_empty(fifo, block)) != APR_SUCCESS) {
        *pelem = NULL;
        return rv;
    }
    *pelem = fifo->elems[fifo->head];
    --fifo->count;
    if (fifo->count > 0) {
        fifo->head = nth_index(fifo, 1);
        if (fifo->count+1 == fifo->nelems) {
            apr_thread_cond_broadcast(fifo->not_full);
        }
    }
    return APR_SUCCESS;
}

static apr_status_t fifo_pull(h2_fifo *fifo, void **pelem, int block)
{
    apr_status_t rv;
    
    if ((rv = apr_thread_mutex_lock(fifo->lock)) == APR_SUCCESS) {
        rv = pull_head(fifo, pelem, block);
        apr_thread_mutex_unlock(fifo->lock);
    }
    return rv;
}

apr_status_t h2_fifo_pull(h2_fifo *fifo, void **pelem)
{
    return fifo_pull(fifo, pelem, 1);
}

apr_status_t h2_fifo_try_pull(h2_fifo *fifo, void **pelem)
{
    return fifo_pull(fifo, pelem, 0);
}

static apr_status_t fifo_peek(h2_fifo *fifo, h2_fifo_peek_fn *fn, void *ctx, int block)
{
    apr_status_t rv;
    void *elem;
    
    if (fifo->aborted) {
        return APR_EOF;
    }
    
    if (APR_SUCCESS == (rv = apr_thread_mutex_lock(fifo->lock))) {
        if (APR_SUCCESS == (rv = pull_head(fifo, &elem, block))) {
            switch (fn(elem, ctx)) {
                case H2_FIFO_OP_PULL:
                    break;
                case H2_FIFO_OP_REPUSH:
                    rv = fifo_push_int(fifo, elem, block);
                    break;
            }
        }
        apr_thread_mutex_unlock(fifo->lock);
    }
    return rv;
}

apr_status_t h2_fifo_peek(h2_fifo *fifo, h2_fifo_peek_fn *fn, void *ctx)
{
    return fifo_peek(fifo, fn, ctx, 1);
}

apr_status_t h2_fifo_try_peek(h2_fifo *fifo, h2_fifo_peek_fn *fn, void *ctx)
{
    return fifo_peek(fifo, fn, ctx, 0);
}

apr_status_t h2_fifo_remove(h2_fifo *fifo, void *elem)
{
    apr_status_t rv;
    
    if (fifo->aborted) {
        return APR_EOF;
    }

    if ((rv = apr_thread_mutex_lock(fifo->lock)) == APR_SUCCESS) {
        int i, rc;
        void *e;
        
        rc = 0;
        for (i = 0; i < fifo->count; ++i) {
            e = fifo->elems[nth_index(fifo, i)];
            if (e == elem) {
                ++rc;
            }
            else if (rc) {
                fifo->elems[nth_index(fifo, i-rc)] = e;
            }
        }
        if (rc) {
            fifo->count -= rc;
            if (fifo->count + rc == fifo->nelems) {
                apr_thread_cond_broadcast(fifo->not_full);
            }
            rv = APR_SUCCESS;
        }
        else {
            rv = APR_EAGAIN;
        }
        
        apr_thread_mutex_unlock(fifo->lock);
    }
    return rv;
}

/*******************************************************************************
 * FIFO int queue
 ******************************************************************************/

struct h2_ififo {
    int *elems;
    int nelems;
    int set;
    int head;
    int count;
    int aborted;
    apr_thread_mutex_t *lock;
    apr_thread_cond_t  *not_empty;
    apr_thread_cond_t  *not_full;
};

static int inth_index(h2_ififo *fifo, int n) 
{
    return (fifo->head + n) % fifo->nelems;
}

static apr_status_t ififo_destroy(void *data) 
{
    h2_ififo *fifo = data;

    apr_thread_cond_destroy(fifo->not_empty);
    apr_thread_cond_destroy(fifo->not_full);
    apr_thread_mutex_destroy(fifo->lock);

    return APR_SUCCESS;
}

static int iindex_of(h2_ififo *fifo, int id)
{
    int i;
    
    for (i = 0; i < fifo->count; ++i) {
        if (id == fifo->elems[inth_index(fifo, i)]) {
            return i;
        }
    }
    return -1;
}

static apr_status_t icreate_int(h2_ififo **pfifo, apr_pool_t *pool, 
                                int capacity, int as_set)
{
    apr_status_t rv;
    h2_ififo *fifo;
    
    fifo = apr_pcalloc(pool, sizeof(*fifo));
    if (fifo == NULL) {
        return APR_ENOMEM;
    }

    rv = apr_thread_mutex_create(&fifo->lock,
                                 APR_THREAD_MUTEX_UNNESTED, pool);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    rv = apr_thread_cond_create(&fifo->not_empty, pool);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    rv = apr_thread_cond_create(&fifo->not_full, pool);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    fifo->elems = apr_pcalloc(pool, capacity * sizeof(int));
    if (fifo->elems == NULL) {
        return APR_ENOMEM;
    }
    fifo->nelems = capacity;
    fifo->set = as_set;
    
    *pfifo = fifo;
    apr_pool_cleanup_register(pool, fifo, ififo_destroy, apr_pool_cleanup_null);

    return APR_SUCCESS;
}

apr_status_t h2_ififo_create(h2_ififo **pfifo, apr_pool_t *pool, int capacity)
{
    return icreate_int(pfifo, pool, capacity, 0);
}

apr_status_t h2_ififo_set_create(h2_ififo **pfifo, apr_pool_t *pool, int capacity)
{
    return icreate_int(pfifo, pool, capacity, 1);
}

apr_status_t h2_ififo_term(h2_ififo *fifo)
{
    apr_status_t rv;
    if ((rv = apr_thread_mutex_lock(fifo->lock)) == APR_SUCCESS) {
        fifo->aborted = 1;
        apr_thread_cond_broadcast(fifo->not_empty);
        apr_thread_cond_broadcast(fifo->not_full);
        apr_thread_mutex_unlock(fifo->lock);
    }
    return rv;
}

int h2_ififo_count(h2_ififo *fifo)
{
    return fifo->count;
}

static apr_status_t icheck_not_empty(h2_ififo *fifo, int block)
{
    while (fifo->count == 0) {
        if (!block) {
            return APR_EAGAIN;
        }
        if (fifo->aborted) {
            return APR_EOF;
        }
        apr_thread_cond_wait(fifo->not_empty, fifo->lock);
    }
    return APR_SUCCESS;
}

static apr_status_t ififo_push_int(h2_ififo *fifo, int id, int block)
{
    if (fifo->aborted) {
        return APR_EOF;
    }

    if (fifo->set && iindex_of(fifo, id) >= 0) {
        /* set mode, elem already member */
        return APR_EEXIST;
    }
    else if (fifo->count == fifo->nelems) {
        if (block) {
            while (fifo->count == fifo->nelems) {
                if (fifo->aborted) {
                    return APR_EOF;
                }
                apr_thread_cond_wait(fifo->not_full, fifo->lock);
            }
        }
        else {
            return APR_EAGAIN;
        }
    }
    
    ap_assert(fifo->count < fifo->nelems);
    fifo->elems[inth_index(fifo, fifo->count)] = id;
    ++fifo->count;
    if (fifo->count == 1) {
        apr_thread_cond_broadcast(fifo->not_empty);
    }
    return APR_SUCCESS;
}

static apr_status_t ififo_push(h2_ififo *fifo, int id, int block)
{
    apr_status_t rv;
    
    if ((rv = apr_thread_mutex_lock(fifo->lock)) == APR_SUCCESS) {
        rv = ififo_push_int(fifo, id, block);
        apr_thread_mutex_unlock(fifo->lock);
    }
    return rv;
}

apr_status_t h2_ififo_push(h2_ififo *fifo, int id)
{
    return ififo_push(fifo, id, 1);
}

apr_status_t h2_ififo_try_push(h2_ififo *fifo, int id)
{
    return ififo_push(fifo, id, 0);
}

static apr_status_t ipull_head(h2_ififo *fifo, int *pi, int block)
{
    apr_status_t rv;
    
    if ((rv = icheck_not_empty(fifo, block)) != APR_SUCCESS) {
        *pi = 0;
        return rv;
    }
    *pi = fifo->elems[fifo->head];
    --fifo->count;
    if (fifo->count > 0) {
        fifo->head = inth_index(fifo, 1);
        if (fifo->count+1 == fifo->nelems) {
            apr_thread_cond_broadcast(fifo->not_full);
        }
    }
    return APR_SUCCESS;
}

static apr_status_t ififo_pull(h2_ififo *fifo, int *pi, int block)
{
    apr_status_t rv;
    
    if ((rv = apr_thread_mutex_lock(fifo->lock)) == APR_SUCCESS) {
        rv = ipull_head(fifo, pi, block);
        apr_thread_mutex_unlock(fifo->lock);
    }
    return rv;
}

apr_status_t h2_ififo_pull(h2_ififo *fifo, int *pi)
{
    return ififo_pull(fifo, pi, 1);
}

apr_status_t h2_ififo_try_pull(h2_ififo *fifo, int *pi)
{
    return ififo_pull(fifo, pi, 0);
}

static apr_status_t ififo_peek(h2_ififo *fifo, h2_ififo_peek_fn *fn, void *ctx, int block)
{
    apr_status_t rv;
    int id;
    
    if (APR_SUCCESS == (rv = apr_thread_mutex_lock(fifo->lock))) {
        if (APR_SUCCESS == (rv = ipull_head(fifo, &id, block))) {
            switch (fn(id, ctx)) {
                case H2_FIFO_OP_PULL:
                    break;
                case H2_FIFO_OP_REPUSH:
                    rv = ififo_push_int(fifo, id, block);
                    break;
            }
        }
        apr_thread_mutex_unlock(fifo->lock);
    }
    return rv;
}

apr_status_t h2_ififo_peek(h2_ififo *fifo, h2_ififo_peek_fn *fn, void *ctx)
{
    return ififo_peek(fifo, fn, ctx, 1);
}

apr_status_t h2_ififo_try_peek(h2_ififo *fifo, h2_ififo_peek_fn *fn, void *ctx)
{
    return ififo_peek(fifo, fn, ctx, 0);
}

static apr_status_t ififo_remove(h2_ififo *fifo, int id)
{
    int rc, i;
    
    if (fifo->aborted) {
        return APR_EOF;
    }

    rc = 0;
    for (i = 0; i < fifo->count; ++i) {
        int e = fifo->elems[inth_index(fifo, i)];
        if (e == id) {
            ++rc;
        }
        else if (rc) {
            fifo->elems[inth_index(fifo, i-rc)] = e;
        }
    }
    if (!rc) {
        return APR_EAGAIN;
    }
    fifo->count -= rc;
    if (fifo->count + rc == fifo->nelems) {
        apr_thread_cond_broadcast(fifo->not_full);
    }
    return APR_SUCCESS;
}

apr_status_t h2_ififo_remove(h2_ififo *fifo, int id)
{
    apr_status_t rv;
    
    if ((rv = apr_thread_mutex_lock(fifo->lock)) == APR_SUCCESS) {
        rv = ififo_remove(fifo, id);
        apr_thread_mutex_unlock(fifo->lock);
    }
    return rv;
}

/*******************************************************************************
 * h2_util for apr_table_t
 ******************************************************************************/
 
typedef struct {
    apr_size_t bytes;
    apr_size_t pair_extra;
} table_bytes_ctx;

static int count_bytes(void *x, const char *key, const char *value)
{
    table_bytes_ctx *ctx = x;
    if (key) {
        ctx->bytes += strlen(key);
    }
    if (value) {
        ctx->bytes += strlen(value);
    }
    ctx->bytes += ctx->pair_extra;
    return 1;
}

apr_size_t h2_util_table_bytes(apr_table_t *t, apr_size_t pair_extra)
{
    table_bytes_ctx ctx;
    
    ctx.bytes = 0;
    ctx.pair_extra = pair_extra;
    apr_table_do(count_bytes, &ctx, t, NULL);
    return ctx.bytes;
}

#endif // H2_UTIL_IMPLEMENTATION

#endif // _ht_util_
