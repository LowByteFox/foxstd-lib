#include <num.h>
#include <alloc.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MUL_NO_OVERFLOW ((size_t) 1 << (sizeof(size_t) * 4))

enum FLAGS {
    CANARY  = 1 << 0,
    FCHECK  = 1 << 1,
    VERBOSE = 1 << 2,
    XMALLOC = 1 << 3,
    LOUD = 1 << 4,
    DUMPC = 1 << 5,
};

const char *fox_alloc_options = NULL;

static u8 alloc_flags = 0 | LOUD; /* make it "loud" */

static bool initialized = false;
static void _fox_alloc_init();
static void _fox_alloc_dump();

static void *(*_malloc)(usize) = malloc;
static void *(*_calloc)(usize, usize) = calloc;
static void *(*_realloc)(void *, usize) = realloc;
static void  (*_free)(void *) = free;

void *fox_alloc(usize size)
{
    if (!initialized) _fox_alloc_init();

    struct foxptr *ptr = _malloc(sizeof(*ptr) + size +
        ((alloc_flags & CANARY) ? 100 : 0));

    if (ptr == NULL) {
        if (alloc_flags & XMALLOC) {
            fprintf(stderr, "Failed to allocate %lu bytes on the heap!\n", size);
            abort();
        }

        return NULL;
    }

    if (alloc_flags & CANARY)
        memset(ptr->data + size, 0, 100);

    if (alloc_flags & LOUD)
        memset(ptr->data, 0xAA, size);

    ptr->allocated = size;

    if (alloc_flags & VERBOSE)
        fprintf(stderr, "Allocated %ld bytes for pointer %p\n", size, ptr->data
            );

    return ptr->data;
}

void *fox_realloc(void *ptr, usize new_size)
{
    if (ptr == NULL)
        return NULL;

    struct foxptr *p = fox_visualize(ptr);
    usize current_size = p->allocated;
    struct foxptr *next = _realloc(p, sizeof(*p) + new_size +
        ((alloc_flags & CANARY) ? 100 : 0));

    if (next == NULL) {
        if (alloc_flags & XMALLOC) {
            fprintf(stderr, "Failed to realloc pointer at %p of size %ld to"
                " %ld bytes on the heap!\n", ptr, p->allocated, new_size);
            abort();
        }

        return NULL;
    }

    if (alloc_flags & CANARY)
        memset(next->data + new_size, 0, 100);

    if (alloc_flags & LOUD)
        memset(next->data + current_size, 0xAA, new_size - current_size);

    next->allocated = new_size;

    if (alloc_flags & VERBOSE)
        fprintf(stderr, "Reallocated pointer %p from %ld bytes to %ld bytes "
            "at pointer %p\n", p->data, current_size, new_size, next->data);

    return next->data;
}

void *fox_recalloc(void *ptr, usize new_size)
{
    if (ptr == NULL)
        return NULL;

    struct foxptr *p = fox_visualize(ptr);
    usize current_size = p->allocated;
    struct foxptr *next = _realloc(p, sizeof(*p) + new_size +
        ((alloc_flags & CANARY) ? 100 : 0));

    if (next == NULL) {
        if (alloc_flags & XMALLOC) {
            fprintf(stderr, "Failed to realloc pointer at %p of size %ld to"
                " %ld bytes on the heap!\n", ptr, p->allocated, new_size);
            abort();
        }

        return NULL;
    }

    memset(next->data + current_size, 0, new_size - current_size);

    if (alloc_flags & CANARY) {
        memset(next->data + new_size, 0, 100);
    }

    next->allocated = new_size;

    if (alloc_flags & VERBOSE)
        fprintf(stderr, "Reallocated pointer %p from %ld bytes to %ld bytes "
            "at pointer %p\n", p->data, current_size, new_size, next->data);

    return next->data;
}

void fox_free(void *ptr)
{
    if (ptr == NULL)
        return;

    if (alloc_flags & CANARY) {
        if (!fox_check(ptr)) {
            fprintf(stderr, "*** heap smashing detected ***: terminated\n");
            abort();
        }
    }

    _free(fox_visualize(ptr));

    if (alloc_flags & VERBOSE)
        fprintf(stderr, "Freed a pointer at address %p\n", ptr);
}

void *fox_reallocarray(void *ptr, usize new_nmemb, usize size)
{
    if ((new_nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
	    new_nmemb > 0 && SIZE_MAX / new_nmemb < size) {
		errno = ENOMEM;
		return NULL;
	}

    return fox_realloc(ptr, new_nmemb * size);
}

void *fox_recallocarray(void *ptr, usize new_nmemb, usize size)
{
    if ((new_nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
	    new_nmemb > 0 && SIZE_MAX / new_nmemb < size) {
		errno = ENOMEM;
		return NULL;
	}

    return fox_recalloc(ptr, new_nmemb * size);
}

void *fox_alloczero(usize size)
{
    if (!initialized) _fox_alloc_init();

    struct foxptr *ptr = _calloc(sizeof(*ptr) + size +
        ((alloc_flags & CANARY) ? 100 : 0), 1);

    if (ptr == NULL) {
        if (alloc_flags & XMALLOC) {
            fprintf(stderr, "Failed to allocate %ld bytes on the heap!\n", size
                );
            abort();
        }

        return NULL;
    }

    ptr->allocated = size;
    if (alloc_flags & VERBOSE)
        fprintf(stderr, "Allocated %ld bytes for pointer %p\n", size, ptr->data
            );

    return ptr->data;
}

void fox_freezero(void *ptr)
{
    if (ptr == NULL)
        return;

    struct foxptr *p = fox_visualize(ptr);

    if (alloc_flags & CANARY) {
        if (!fox_check(ptr)) {
            fprintf(stderr, "*** heap smashing detected ***: terminated\n");
            abort();
        }
    }

    memset(p, 0, sizeof(*p) + p->allocated);

    _free(p);
    if (alloc_flags & VERBOSE)
        fprintf(stderr, "Freed a pointer at address %p\n", ptr);
}

bool fox_check(void *ptr)
{
    if (ptr == NULL)
        return true;

    struct foxptr *p = fox_visualize(ptr);
    u8 *iter = p->data + p->allocated;
    i32 i = 0;

    for (; i < 100; i++, iter++) {
        if (*iter) return false;
    }

    return true;
}

usize fox_allocated(void *ptr)
{
    struct foxptr *p = fox_visualize(ptr);

    return p->allocated;
}

void fox_set_malloc(void *(*fn)(usize))
{
    _malloc = fn;
}

void fox_set_calloc(void *(*fn)(usize, usize))
{
    _calloc = fn;
}

void fox_set_realloc(void *(*fn)(void *, usize))
{
    _realloc = fn;
}

void fox_set_free(void (*fn)(void *))
{
    _free = fn;
}

static void _fox_alloc_init()
{
    initialized = true;
    if (fox_alloc_options == NULL)
        return;
    const char *opts = fox_alloc_options;

    while (*fox_alloc_options) {
        switch (*fox_alloc_options) {
        case 'C':
            alloc_flags |= CANARY;
            break;
        case 'D':
            atexit(_fox_alloc_dump);
            break;
        case '+':
            if (fox_alloc_options - 1 >= opts) {
                if (*(fox_alloc_options - 1) != 'D') {
                    fprintf(stderr, "Warning: option + cannot be used on flag "
                        "'%c'\n", *(fox_alloc_options - 1));
                    break;
                }
                if (alloc_flags & LOUD)
                    fprintf(stderr, "Warning: dumping memory content into the "
                        "dump file can expose sensitive data\n");
                alloc_flags |= DUMPC;
            } else
                fprintf(stderr, "Warning: option + cannot be used alone\n");
            break;
        case 'F':
            alloc_flags |= FCHECK;
            break;
        case 'Q':
            alloc_flags &= ~LOUD;
            break;
        case 'V':
            alloc_flags |= VERBOSE;
            break;
        case 'X':
            alloc_flags |= XMALLOC;
            break;
        default:
            fprintf(stderr, "Unknown fox_alloc_option '%c'!\n",
                *fox_alloc_options);
            abort();
        }

        fox_alloc_options++;
    }
}

static void _fox_alloc_dump()
{
    printf("dumping nothing yet\n");
}
