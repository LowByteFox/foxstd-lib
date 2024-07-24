#include <alloc.h>
#include <string.h>
#include <num.h>
#include <fns.h>
#include <vec.h>

struct fox_vec fox_vec_new(const usize chunksize)
{
    struct fox_vec vec = { .chunksize = chunksize, 0 };
    vec.items = fox_reallocarray(vec.items, 16, chunksize);
    return vec;
}

void fox_vec_del(struct fox_vec *vec, const deletor *deletor)
{
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
    usize cap = fox_allocated(vec->items) / vec->chunksize;
    if (cap == vec->size)
        vec->items = fox_reallocarray(vec->items, vec->size * 2,
            vec->chunksize);

    u8 *iter = vec->items;

    memcpy(iter + vec->size * vec->chunksize, data, vec->chunksize);
    vec->size++;
}

void fox_vec_insert(struct fox_vec *vec, const usize index, void *data);
void            fox_vec_fill(struct fox_vec *vec, void *data);
void            fox_vec_rotate(struct fox_vec *vec, const isize rotation);
void*           fox_vec_front(const struct fox_vec *vec);
void*           fox_vec_back(const struct fox_vec *vec);
void*           fox_vec_get(const struct fox_vec *vec, const usize index);
void            fox_vec_pop(struct fox_vec *vec);
void            fox_vec_remove(struct fox_vec *vec);
void            fox_vec_reserve(struct fox_vec *vec, const usize capacity);
void            fox_vec_shrink_to_fit(struct fox_vec *vec);
bool            fox_vec_is_empty(const struct fox_vec *vec);
bool            fox_vec_is_sorted(const struct fox_vec *vec,
                    const comparar *comparar);
isize           fox_vec_find(const struct fox_vec *vec, const void *needle,
                    const comparar *comparar);
void            fox_vec_swap(struct fox_vec *vec, struct fox_vec *vec2);
