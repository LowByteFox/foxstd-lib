#include <alloc.h>
#include <assert.h>
#include <utils.h>
#include <string.h>
#include <num.h>
#include <fns.h>
#include <vec.h>

struct fox_vec fox_vec_new(const usize chunksize)
{
    assert(chunksize > 0);
    struct fox_vec vec = { .chunksize = chunksize, 0 };
    vec.items = fox_reallocarray(vec.items, 16, chunksize);
    return vec;
}

void fox_vec_del(struct fox_vec *vec, deletor *deletor)
{
    assert(vec != NULL);

    u8 *iter = vec->items;

    for (usize i = 0; i < vec->size; i++) {
        if (deletor != NULL)
            deletor(iter);
        iter += vec->chunksize;
    }

    fox_free(vec->items);
}

void fox_vec_push(struct fox_vec *vec, void *data)
{
    assert(vec != NULL);
    assert(data != NULL);

    usize cap = fox_allocated(vec->items) / vec->chunksize;
    assert(cap > vec->size);
    if (cap == vec->size)
        vec->items = fox_reallocarray(vec->items, vec->size * 2,
            vec->chunksize);

    u8 *iter = vec->items;

    memcpy(iter + vec->size * vec->chunksize, data, vec->chunksize);
    vec->size++;
}

void fox_vec_insert(struct fox_vec *vec, const usize index, void *data)
{
    assert(vec != NULL);
    assert(data != NULL);

    if (index >= vec->size) {
        fox_vec_push(vec, data);
        return;
    }

    usize cap = fox_allocated(vec->items) / vec->chunksize;
    assert(cap > vec->size);
    if (cap == vec->size)
        vec->items = fox_reallocarray(vec->items, vec->size * 2,
            vec->chunksize);

    u8 *iter = vec->items;
    fox_rmemcpy(iter + vec->chunksize * (index + 1),
        iter + vec->chunksize * index,
        (vec->size - index) * vec->chunksize);

    memcpy(iter + index * vec->chunksize, data, vec->chunksize);
    vec->size++;
}

void fox_vec_fill(struct fox_vec *vec, void *data)
{
    assert(vec != NULL);
    assert(data != NULL);

    usize cap = fox_allocated(vec->items) / vec->chunksize;
    assert(cap > vec->size);

    if (cap == vec->size)
        return;

    for (usize i = vec->size; i < cap; i++)
        memcpy(vec->items + i * vec->chunksize, data, vec->chunksize);

    vec->size = cap;
}

void fox_vec_rotate(struct fox_vec *vec, const isize rotation) 
{
    assert(vec != NULL);
    u8 *iter = vec->items;

    if (rotation < 0) {
        u8 data_buffer[vec->chunksize + (-rotation)];
        memcpy(data_buffer, iter, vec->chunksize * (-rotation));
        memcpy(iter, iter + (-rotation) * vec->chunksize,
            (vec->size - (-rotation)) * vec->chunksize);
        memcpy(iter + (vec->size - (-rotation)) * vec->chunksize,
            data_buffer, vec->chunksize * (-rotation));
    } else if (rotation > 0) {
        u8 data_buffer[vec->chunksize + rotation];
        memcpy(data_buffer, iter + (vec->size - rotation) * vec->chunksize,
            vec->chunksize * rotation);
        fox_rmemcpy(iter + rotation * vec->chunksize, iter,
            (vec->size - rotation) * vec->chunksize);
        memcpy(iter, data_buffer, vec->chunksize * rotation);
    }
}

void *fox_vec_front(const struct fox_vec *vec)
{
    assert(vec != NULL);
    if (vec->size == 0)
        return NULL;

    return vec->items;
}

void *fox_vec_back(const struct fox_vec *vec)
{
    assert(vec != NULL);
    if (vec->size == 0)
        return NULL;

    return vec->items + (vec->size - 1) * vec->chunksize;
}

void *fox_vec_get(const struct fox_vec *vec, const usize index)
{
    assert(vec != NULL);
    if (index >= vec->size)
        return NULL;

    return vec->items + index * vec->chunksize;
}

void fox_vec_pop(struct fox_vec *vec, deletor *deletor)
{
    assert(vec != NULL);
    if (vec->size == 0)
        return;

    if (deletor != NULL)
        deletor(fox_vec_back(vec));

    vec->size--;
}

void fox_vec_remove(struct fox_vec *vec, const usize index, deletor *deletor)
{
    assert(vec != NULL);
    if (index >= vec->size)
        return;

    u8 *iter = vec->items;

    if (deletor != NULL)
        deletor(iter + vec->chunksize * index);

    memcpy(iter + vec->chunksize * index,
        iter + vec->chunksize * (index + 1),
        (vec->size - index - 1) * vec->chunksize);

    vec->size--;
}

void fox_vec_reserve(struct fox_vec *vec, const usize capacity)
{
    assert(vec != NULL);
    usize cap = fox_allocated(vec->items) / vec->chunksize;

    if (cap >= capacity)
        return;

    vec->items = fox_reallocarray(vec->items, capacity,
        vec->chunksize);
}

void fox_vec_shrink_to_fit(struct fox_vec *vec)
{
    assert(vec != NULL);
    usize cap = fox_allocated(vec->items) / vec->chunksize;

    vec->items = fox_reallocarray(vec->items, cap,
        vec->chunksize);
}

bool fox_vec_is_empty(const struct fox_vec *vec)
{
    return vec->size == 0;
}

bool fox_vec_is_sorted(const struct fox_vec *vec, comparar *comparar)
{
    assert(vec != NULL);

    u8 *iter = vec->items;

    for (usize i = 0; i < vec->size - 1; i++) {
        if (comparar != NULL) {
            if (!comparar(iter, iter + vec->chunksize))
                return false;
        } else {
            if (memcmp(iter, iter + vec->chunksize, vec->chunksize) > 0)
                return false;
        }

        iter += vec->chunksize;
    }

    return true;
}

isize fox_vec_find(const struct fox_vec *vec, const void *needle,
    comparar *comparar)
{
    assert(vec != NULL);

    u8 *iter = vec->items;

    for (usize i = 0; i < vec->size; i++) {
        if (comparar != NULL) {
            if (comparar(iter, needle))
                return i;
        } else {
            if (memcmp(iter, needle, vec->chunksize) == 0)
                return i;
        }

        iter += vec->chunksize;
    }

    return -1;
}

void fox_vec_swap(struct fox_vec *vec, struct fox_vec *vec2)
{
    struct fox_vec tmp = *vec;
    *vec = *vec2;
    *vec2 = tmp;
}
