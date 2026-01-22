#include <assert.h>
#include <string.h>

#define da_append(xs, x)                                                       \
    do {                                                                       \
        if ((xs).len + 1 > (xs).cap) {                                         \
            if ((xs).cap != 0) {                                               \
                (xs).cap *= 2;                                                 \
            } else {                                                           \
                (xs).cap = 4;                                                  \
            }                                                                  \
            (xs).data = realloc((xs).data, sizeof(*(xs).data) * (xs).cap);     \
            assert((xs).data != NULL);                                         \
        }                                                                      \
        (xs).data[(xs).len++] = (x);                                           \
    } while (0)

#define da_last(xs) (xs).data[(xs).len - 1]

#define da_append_empty(xs)                                                    \
    do {                                                                       \
        da_reserve((xs), 1);                                                   \
        (xs).len++;                                                            \
        memset(&da_last((xs)), 0, sizeof(*(xs).data));                         \
    } while (0)

#define da_reserve(xs, n)                                                      \
    do {                                                                       \
        if ((xs).len + (n) > (xs).cap) {                                       \
            if ((xs).cap == 0) {                                               \
                (xs).cap = 4;                                                  \
            }                                                                  \
            while ((xs).len + (n) > (xs).cap) {                                \
                (xs).cap *= 2;                                                 \
            }                                                                  \
            (xs).data = realloc((xs).data, sizeof(*(xs).data) * (xs).cap);     \
            assert((xs).data != NULL);                                         \
        }                                                                      \
    } while (0)

#define da_append_many(xs, ys, n)                                              \
    do {                                                                       \
        da_reserve((xs), (n));                                                 \
        memcpy(&(xs).data[(xs).len], (ys), (n) * sizeof(*(ys)));               \
        (xs).len += (n);                                                       \
    } while (0)

#define da_append_str(xs, str)                                                 \
    do {                                                                       \
        da_reserve((xs), strlen(str));                                         \
        strcpy(&(xs).data[(xs).len], str);                                     \
        (xs).len += strlen(str) + 1;                                           \
    } while (0)
