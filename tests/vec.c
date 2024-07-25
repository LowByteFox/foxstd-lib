#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <vec.h>
#include <num.h>

bool comp(const void *a, const void *b)
{
    return *(const int*) a <= *(const int*) b;
}

bool comp2(const void *a, const void *b)
{
    return *(const int*) a == *(const int*) b;
}

int compare(const void* a, const void* b)
{
    return *(const int*) a - *(const int*) b;
}

int main()
{
    struct fox_vec vec = fox_vec_new(sizeof(int));
    int val = 3;

    fox_vec_push(&vec, &val); /* [3] */
    val = 4;
    fox_vec_push(&vec, &val); /* [3, 4] */
    val = 7;
    fox_vec_push(&vec, &val); /* [3, 4, 7] */

    val = 10;
    fox_vec_insert(&vec, 0, &val); /* [10, 3, 4, 7] */
    val = 11;
    fox_vec_insert(&vec, 2, &val); /* [10, 3, 11, 4, 7] */

    val = 77;
    fox_vec_fill(&vec, &val);
    /* [10, 3, 11, 4, 7, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77] */

    fox_vec_rotate(&vec, -2);
    /* [11, 4, 7, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 10, 3] */

    fox_vec_rotate(&vec, 4);
    /* [77, 77, 10, 3, 11, 4, 7, 77, 77, 77, 77, 77, 77, 77, 77, 77] */

    assert(*(int*) fox_vec_front(&vec) == 77);
    assert(*(int*) fox_vec_back(&vec) == 77);
    assert(*(int*) fox_vec_get(&vec, 4) == 11);

    fox_vec_remove(&vec, 0, NULL);
    fox_vec_remove(&vec, 4, NULL);
    /* [77, 10, 3, 11, 7, 77, 77, 77, 77, 77, 77, 77, 77, 77] */

    fox_vec_reserve(&vec, 19);
    val = 5;
    fox_vec_fill(&vec, &val);
    /* [77, 10, 3, 11, 7, 77, 77, 77, 77, 77, 77, 77, 77, 77, 5, 5, 5, 5, 5] */

    assert(fox_vec_is_sorted(&vec, comp) == 0);
    qsort(vec.items, vec.size, sizeof(int), compare);
    assert(fox_vec_is_sorted(&vec, comp) == 1);

    val = 11;
    assert(fox_vec_find(&vec, &val, comp2) != -1);
    /* reusing */
    val = fox_vec_find(&vec, &val, comp2);
    assert(*(int*) fox_vec_get(&vec, val) == 11);
    
    int *iter = vec.items;
    for (int i = 0; i < vec.size; i++, iter++)
        printf("%d\n", *iter);

    fox_vec_del(&vec, NULL);
    return 0;
}
