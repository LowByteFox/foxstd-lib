#include <num.h>
#include <alloc.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STACK_SIZE 128
#define MUL_NO_OVERFLOW ((size_t) 1 << (sizeof(size_t) * 4))

enum FLAGS {
    CANARY  = 1 << 0,
    FCHECK  = 1 << 1,
    VERBOSE = 1 << 2,
    XMALLOC = 1 << 3,
    LOUD = 1 << 4,
    DUMPC = 1 << 5,
};

/* private hashmap start */
enum _state {
    EMPTY,
    VALID,
    DELETED
};

struct _allocation_info {
    /* void *trace[STACK_SIZE]; */
    bool freed;
};

struct _hashmap_pair {
    enum _state state;
    void *key;
    struct _allocation_info value;
};

struct _hashmap {
    usize len;
    usize cap;
    struct _hashmap_pair *pairs;
};

static u32      _djb2(const void *bytes, usize len);
static usize    hashmap_insert(struct _hashmap *hm, void *key,
                    struct _allocation_info *info);
static void     hashmap_remove(struct _hashmap *hm, usize it);
static usize    hashmap_find(const struct _hashmap *hm, void *key);
static bool     hashmap_resize(struct _hashmap *hm);
/* private hashmap end */

const char *fox_alloc_options = NULL;

static u8 alloc_flags = 0 | LOUD; /* make it "loud" */
static struct _hashmap table = {0};

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

    if (alloc_flags & DUMPC || alloc_flags & FCHECK) {
        struct _allocation_info info = {0};
        info.freed = false;
        hashmap_insert(&table, ptr, &info);
    }

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

    if (alloc_flags & DUMPC || alloc_flags & FCHECK)
        if (p != next) {
            struct _allocation_info info = table.pairs[hashmap_find(&table, p)].value;
            hashmap_remove(&table, hashmap_find(&table, p));
            hashmap_insert(&table, ptr, &info);
        }

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

    if (alloc_flags & DUMPC || alloc_flags & FCHECK)
        if (p != next) {
            struct _allocation_info info = table.pairs[hashmap_find(&table, p)].value;
            hashmap_remove(&table, hashmap_find(&table, p));
            hashmap_insert(&table, ptr, &info);
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

    struct foxptr *p = fox_visualize(ptr);

    if (alloc_flags & DUMPC || alloc_flags & FCHECK) {
        struct _allocation_info info = table.pairs[hashmap_find(&table, p)].value;
        if (info.freed && alloc_flags & FCHECK) {
            fprintf(stderr, "*** double free detected ***: terminated\n");
            abort();
        }

        table.pairs[hashmap_find(&table, p)].value.freed = true;
    }

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

    if (alloc_flags & DUMPC || alloc_flags & FCHECK) {
        struct _allocation_info info = {0};
        info.freed = false;
        hashmap_insert(&table, ptr, &info);
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

    if (alloc_flags & DUMPC || alloc_flags & FCHECK) {
        struct _allocation_info info = table.pairs[hashmap_find(&table, p)].value;
        if (info.freed && alloc_flags & FCHECK) {
            fprintf(stderr, "*** double free detected ***: terminated\n");
            abort();
        }

        table.pairs[hashmap_find(&table, p)].value.freed = true;
    }

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
    int magic = 0, screaming = 0;

    FILE *dump_file = fopen("memdump.foxstd", "w");
    if (alloc_flags & DUMPC)
        magic = 1;

    fwrite(&magic, sizeof(int), 1, dump_file);

    for (usize i = 0; i < table.cap; i++) {
        struct _hashmap_pair *it = table.pairs + i;
        if (it->state == VALID && !it->value.freed) {
            struct foxptr *ptr = it->key;

            if (alloc_flags & LOUD) {
                screaming = 1;
                u8 *data = ptr->data;
                for (usize j = 0; j < ptr->allocated; j++) {
                    if (data[j] != 0xAA) {
                        screaming = 0;
                        break;
                    }
                }
            }

            /* screaming, address, size, content */

            void **addr = (void*) &ptr->data;

            fwrite(&screaming, sizeof(int), 1, dump_file);
            fwrite(&addr, sizeof(void*), 1, dump_file);
            fwrite(&ptr->allocated, sizeof(usize), 1, dump_file);

            if (magic)
                fwrite(ptr->data, sizeof(u8), ptr->allocated, dump_file);
        }
    }

    fclose(dump_file);
}

/* private hashmap start */
static u32 _djb2(const void *bytes, usize len)
{
    const u8 *ptr = bytes;
    u32 hash = 5381;
    for (usize i = 0; i < len; ++i)
        hash = hash * 33 + ptr[i];

    return hash;
}

static usize hashmap_insert(struct _hashmap *hm, void *key,
    struct _allocation_info *info)
{
    if (!hashmap_resize(hm))
        return hm->cap;

    size_t it = _djb2(&key, sizeof(void*)) % hm->cap;

    while (hm->pairs[it].state == VALID && memcmp(&key, &hm->pairs[it].key,
        sizeof(void*)))
        it = (it + 1) % hm->cap;

    if (hm->pairs[it].state != VALID)
        hm->len += 1;

    hm->pairs[it].state = VALID;
    hm->pairs[it].key = key;
    hm->pairs[it].value = *info;

    return it;
}

static usize hashmap_find(const struct _hashmap *hm, void *key)
{
    if (hm->cap == 0)
        return hm->cap;

    usize it = _djb2(&key, sizeof(void*)) % hm->cap;

    while (hm->pairs[it].state == VALID && memcmp(&key, &hm->pairs[it].key,
        sizeof(void*)))
            it = (it + 1) % hm->cap;

    if (hm->pairs[it].state != VALID)
        return hm->cap;

    return it;
}

static void hashmap_remove(struct _hashmap *hm, size_t it)
{
    hm->pairs[it].state = DELETED;
    hm->len -= 1;
    hashmap_resize(hm);
}

static bool hashmap_resize(struct _hashmap *hm)
{
    size_t oldCap = hm->cap;
    size_t newCap;

    if (!hm->cap || hm->len * 4 > hm->cap * 3) {
        newCap = oldCap > 0 ? oldCap * 2 : 128;
    } else if (hm->cap > 128 && hm->len * 4 < hm->cap) {
        newCap = oldCap / 2;
    } else {
        return true;
    }

    struct _hashmap_pair *new_pairs = calloc(newCap, sizeof(*new_pairs));

    if (!new_pairs)
        return false;

    for (size_t i = 0; i < oldCap; ++i) {
        if (hm->pairs[i].state != VALID)
            continue;
        size_t it = _djb2(&hm->pairs[i].key, sizeof(void*)) % newCap;
        while (new_pairs[it].state == VALID)
            it = (it + 1) % newCap;

        new_pairs[it].state = VALID;
        new_pairs[it].key = hm->pairs[i].key;
        new_pairs[it].value = hm->pairs[i].value;
    }

    free(hm->pairs);
    hm->pairs = new_pairs;
    hm->cap = newCap;

    return true;
}
/* private hashmap end */
