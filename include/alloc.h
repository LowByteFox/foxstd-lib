#pragma once

#include <num.h>

struct foxptr {
    usize allocated;
    u8 data[];
};

/*
 * C    “Canaries”. Add canaries at the end of allocations in order to detect
 *      heap overflows.
 *
 * D    "Dump". Foxstd will dump a leak report at exit into a file, this option
 *      can be followed with + to dump memory content
 *
 * F    "Freecheck". Enable more extensive double free detection.
 *
 * Q    "Quiet". Foxstd will not make the data "scream" and won't show warning
 *      when dumping the memory content as well
 *
 * V    "Verbose". Foxstd will print every information during runtime.
 *
 * X    "xmalloc". Rather than return failure, abort the program with a
 *      diagnostic message on stderr.
 *
 * Options can be combined
 */
extern const char *fox_alloc_options;

void*   fox_alloc(usize size);
void*   fox_realloc(void *ptr, usize new_size);
void*   fox_recalloc(void *ptr, usize new_size);
void    fox_free(void *ptr);

void*   fox_reallocarray(void *ptr, usize new_nmemb, usize size);
void*   fox_recallocarray(void *ptr, usize new_nmemb, usize size);

void*   fox_alloczero(usize size);
void    fox_freezero(void *ptr);
bool    fox_check(void *ptr);

#define fox_visualize(ptr) ((struct foxptr*) (((u8*) ptr) - sizeof(usize)))
usize   fox_allocated(void *ptr);

void    fox_set_malloc(void *(*fn)(usize));
void    fox_set_calloc(void *(*fn)(usize, usize));
void    fox_set_realloc(void *(*fn)(void *, usize));
void    fox_set_free(void (*fn)(void *));
