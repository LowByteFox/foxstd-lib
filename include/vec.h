#pragma once

#include <num.h>
#include <fns.h>

struct fox_vec {
    const usize chunksize;
    usize size;
    void *items;
};

struct fox_vec  fox_vec_new(const usize chunksize);
void            fox_vec_del(struct fox_vec *vec, deletor *deletor);
void            fox_vec_push(struct fox_vec *vec, void *data);
void            fox_vec_insert(struct fox_vec *vec, const usize index,
                    void *data);
void            fox_vec_fill(struct fox_vec *vec, void *data);
void            fox_vec_rotate(struct fox_vec *vec, const isize rotation);
void*           fox_vec_front(const struct fox_vec *vec);
void*           fox_vec_back(const struct fox_vec *vec);
void*           fox_vec_get(const struct fox_vec *vec, const usize index);
void            fox_vec_pop(struct fox_vec *vec, deletor *deletor);
void            fox_vec_remove(struct fox_vec *vec, const usize index,
                    deletor *deletor);
void            fox_vec_reserve(struct fox_vec *vec, const usize capacity);
void            fox_vec_shrink_to_fit(struct fox_vec *vec);
bool            fox_vec_is_empty(const struct fox_vec *vec);
bool            fox_vec_is_sorted(const struct fox_vec *vec,
                    comparar *comparar);
isize           fox_vec_find(const struct fox_vec *vec, const void *needle,
                    comparar *comparar);
void            fox_vec_swap(struct fox_vec *vec, struct fox_vec *vec2);
