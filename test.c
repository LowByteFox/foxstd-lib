#include <alloc.h>

#include <stdio.h>

int main() {
    fox_alloc_options = "CVXFD";

    int *ptr = fox_alloc(sizeof(int));

    printf("%X\n", *ptr);
    return 0;
}
