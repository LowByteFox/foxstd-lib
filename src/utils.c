#include <assert.h>
#include <num.h>
#include <utils.h>

void *fox_rmemcpy(void *dest, const void *src, usize n)
{
    if (dest == NULL || src == NULL)
        return NULL;

    u8 *d = dest;
    const u8 *s = src;

    d += n - 1;
    s += n - 1;

    while (n--) {
        *d-- = *s--;
    }

    return dest;
}
