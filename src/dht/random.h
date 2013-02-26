#ifndef _random_h
#define _random_h

#include <stdint.h>
#include <stdlib.h>

typedef struct RandomState {
    char *buf;
    size_t buf_len;
    struct random_data *data;
} RandomState;

RandomState *RandomState_Create(int seed);
void RandomState_Destroy(RandomState *randomState);

int Random_Fill(RandomState *randomState, char *buf, size_t len);

#endif

