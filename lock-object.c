/* Copyright (c) 2013, Robert Escriva
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Replicant nor the names of its contributors may be
 *       used to endorse or promote products derived from this software without
 *       specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* C */
#include <assert.h>
#include <ctype.h>
#include <string.h>

/* Replicant */
#include <replicant_state_machine.h>

#define UNUSED(X) (void)(X)

#define DESCRIPTION_LENGTH 64
#define RESPONSE_BUFFER_SIZE (DESCRIPTION_LENGTH + 20/*strlen(UINT64_MAX)*/ + 4)

/* The state encapsulated within our distributed lock */
struct distributed_lock
{
    uint64_t next_count;
    struct lock_holder* holder;
    struct lock_holder** tail;
    char response_buffer[RESPONSE_BUFFER_SIZE];
};

struct lock_holder
{
    uint64_t count;
    struct lock_holder* next;
    char* name;
};

static void
pop_first_lock_holder(struct lock_holder** head, struct lock_holder*** tail)
{
    struct lock_holder* next = (*head)->next;

    /* free lh */
    if ((*head)->name)
    {
        free((*head)->name);
    }

    free(*head);

    /* set next */
    if (next)
    {
        *head = next;
    }
    else
    {
        *head = NULL;
        *tail = head;
    }
}

/* Code used to create the lock. */
void*
lock_create(struct replicant_state_machine_context* ctx)
{
    struct distributed_lock* lock = malloc(sizeof(struct distributed_lock));

    if (lock)
    {
        lock->next_count = 0;
        lock->holder = NULL;
        lock->tail = &lock->holder;

        if (replicant_state_machine_condition_create(ctx, "wake") < 0)
        {
            free(lock);
            lock = NULL;
        }
    }

    return lock;
}

/* Cleanup memory when the lock is destroyed. */
void
lock_destroy(struct replicant_state_machine_context* ctx, void* obj)
{
    struct distributed_lock* lock = (struct distributed_lock*) obj;

    while (lock->holder)
    {
        pop_first_lock_holder(&lock->holder, &lock->tail);
    }

    free(lock);
    UNUSED(ctx);
}

/* Take a snapshot of the current state of the lock.
 * Not implemented because they'd be meaningless.
 */
void
lock_snapshot(struct replicant_state_machine_context* ctx,
             void* obj,
             const char** data, size_t* data_sz)
{
    *data = NULL;
    *data_sz = 0;
    UNUSED(ctx);
    UNUSED(obj);
}

/* Create a new lock using the snapshot's data.
 * Not implemented because snapshots are not implemented.
 */
void*
lock_recreate(struct replicant_state_machine_context* ctx,
              const char* data, size_t data_sz)
{
    UNUSED(ctx);
    UNUSED(data);
    UNUSED(data_sz);
    return NULL;
}

/* Enqueue the requester on the queue */
void
lock_lock(struct replicant_state_machine_context* ctx,
          void* obj, const char* data, size_t data_sz)
{
    FILE* log = replicant_state_machine_log_stream(ctx);
    struct distributed_lock* lock = (struct distributed_lock*) obj;
    struct lock_holder* lh = NULL;

    /* Initialize the struct lock_holder */
    lh = (struct lock_holder*) malloc(sizeof(struct lock_holder));
    assert(lh);
    lh->count = lock->next_count;
    ++lock->next_count;
    lh->name = NULL;
    lh->next = NULL;

    /* Copy the name */
    /* first trim the data_sz to be less than DESCRIPTION_LENGTH */
    data_sz = strnlen(data, data_sz);
    data_sz = data_sz <= DESCRIPTION_LENGTH ? data_sz : DESCRIPTION_LENGTH;
    /* allocate a new buffer to describe the lock holder */
    lh->name = (char*) malloc(data_sz + 1);
    /* copy the data and make sure the string is null-terminated */
    lh->name[data_sz] = '\0';
    strncpy(lh->name, data, data_sz);

    /* Enqueue the struct lock_holder on our list of lock holders */
    *lock->tail = lh;
    lock->tail = &lh->next;

    /* Send the response, saying when we're locked */
    snprintf(lock->response_buffer, RESPONSE_BUFFER_SIZE,
             "%s@%lu", lh->name, lh->count);
    replicant_state_machine_set_response(ctx,
                                         lock->response_buffer,
                                         strlen(lock->response_buffer) + 1);

    /* log about the request to acquire */
    if (lock->holder == lh)
    {
        fprintf(log, "lock acquired by %s@%lu\n",
                     lh->name, lh->count);
    }
    else
    {
        fprintf(log, "%s waiting until %lu to acquire the lock\n",
                     lh->name, lh->count);
    }
}

void
lock_unlock(struct replicant_state_machine_context* ctx,
            void* obj, const char* data, size_t data_sz)
{
    FILE* log = replicant_state_machine_log_stream(ctx);
    struct distributed_lock* lock = (struct distributed_lock*) obj;
    char name[DESCRIPTION_LENGTH];
    uint64_t count = 0;
    uint64_t wakeup = 0;
    int matched = 0;

    /* ensure that data_sz is NULL-terminated */
    if (strnlen(data, data_sz) >= data_sz)
    {
        fprintf(log, "unlock invalid: not NULL-terminated\n");
        return;
    }

    matched = sscanf(data, "%64[^@]@%lu", name, &count);

    if (matched != 2)
    {
        fprintf(log, "unlock invalid: should be \"name@count\"\n");
        return;
    }

    if (!lock->holder)
    {
        fprintf(log, "out of order unlock: got %s@%lu, "
                     "but nothing holds the lock\n", name, count);
        replicant_state_machine_set_response(ctx, "fail", 4);
        return;
    }

    if (strcmp(name, lock->holder->name) != 0 ||
        count != lock->holder->count)
    {
        fprintf(log, "out of order unlock: got %s@%lu, expected %s@%lu\n",
                     name, count, lock->holder->name, lock->holder->count);
        replicant_state_machine_set_response(ctx, "fail", 4);
        return;
    }

    /* log about the unlock */
    fprintf(log, "lock released by %s@%lu\n",
                 lock->holder->name, lock->holder->count);
    /* destroy the lock_holder instance */
    pop_first_lock_holder(&lock->holder, &lock->tail);

    if (lock->holder)
    {
        fprintf(log, "lock acquired by %s@%lu\n",
                     lock->holder->name, lock->holder->count);
        wakeup = lock->holder->count;
    }
    else
    {
        wakeup = lock->next_count;
    }

    while (count < wakeup)
    {
        replicant_state_machine_condition_broadcast(ctx, "wake", &count);
    }

    replicant_state_machine_set_response(ctx, "success", 7);
    return;
}

void
lock_holder(struct replicant_state_machine_context* ctx,
            void* obj, const char* data, size_t data_sz)
{
    struct distributed_lock* lock = (struct distributed_lock*) obj;

    if (lock->holder)
    {
        snprintf(lock->response_buffer, RESPONSE_BUFFER_SIZE,
                 "%s@%lu", lock->holder->name, lock->holder->count);
    }
    else
    {
        snprintf(lock->response_buffer, RESPONSE_BUFFER_SIZE,
                 "not held");
    }

    replicant_state_machine_set_response(ctx,
                                         lock->response_buffer,
                                         strlen(lock->response_buffer) + 1);
    UNUSED(data);
    UNUSED(data_sz);
}

struct replicant_state_machine rsm = {
    lock_create,
    lock_recreate,
    lock_destroy,
    lock_snapshot,
    {{"lock", lock_lock},
     {"unlock", lock_unlock},
     {"holder", lock_holder},
     {NULL, NULL}}
};
