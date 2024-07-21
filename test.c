#include <alloc.h>

#include <stdio.h>

int main() {
    fox_alloc_options = "CVXF";

    int *ptr = fox_alloc(sizeof(int));

    printf("%X\n", *ptr);

    fox_free(ptr);
    return 0;
}
