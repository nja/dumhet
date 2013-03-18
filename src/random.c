#include <stdint.h>
#include <stdlib.h>

#include <dht/random.h>
#include <lcthw/dbg.h>

#define RANDOM_STATE_LEN 256

RandomState *RandomState_Create(int seed)
{
    RandomState *state = NULL;
    char *buf = NULL;
    struct random_data *data = NULL;
    
    state = calloc(1, sizeof(RandomState));
    check_mem(state);

    buf = malloc(RANDOM_STATE_LEN);
    check_mem(buf);

    data = calloc(1, sizeof(struct random_data));
    check_mem(data);

    int rc = initstate_r(seed, buf, RANDOM_STATE_LEN, data);
    check(rc == 0, "Failed to initialize random state");

    state->buf = buf;
    state->buf_len = RANDOM_STATE_LEN;
    state->data = data;

    return state;

error:
    free(state);
    free(buf);
    free(data);

    return NULL;
}

void RandomState_Destroy(RandomState *randomState)
{
    if (randomState == NULL)
        return;

    free(randomState->buf);
    free(randomState->data);
    free(randomState);
}

int Random_Fill(RandomState *randomState, char *buf, size_t len)
{
    size_t i = 0;
    int rc = 0;

    for (i = 0; i + sizeof(int32_t) <= len; i += sizeof(int32_t))
    {
	rc = random_r(randomState->data, (int32_t *)&buf[i]);
	check(rc == 0, "Random number generation error");
    }

    if (i == len) return 0;

    int32_t r = 0;
    rc = random_r(randomState->data, &r);
    check(rc == 0, "Random number generation error");

    while (i < len)
    {
	buf[i++] = r & 0xff;
	r = r >> 8;
    }

    return 0;
error:
    return -1;
}
