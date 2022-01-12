#ifndef TYPES_H
#define TYPES_H

typedef enum {
    SUCCESS = 0,
    FAILURE = -1,
    BLOCKED = 1,
    INTERRUPTED = 2,
    TIMEOUT = 3,
    LOCKED = 4
} RetType;

#endif
