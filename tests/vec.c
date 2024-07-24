#include <stdio.h>
#include <vec.h>

int main()
{
    struct fox_vec vec = fox_vec_new(sizeof(int));
    int val = 3;

    fox_vec_push(&vec, &val);
    val = 4;
    fox_vec_push(&vec, &val);
    val = 7;
    fox_vec_push(&vec, &val);

    int *iter = vec.items;
    for (int i = 0; i < vec.size; i++, iter++)
        printf("%d\n", *iter);

    fox_vec_del(&vec, NULL);
    return 0;
}
