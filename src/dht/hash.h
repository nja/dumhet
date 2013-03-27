#ifndef _hash_h
#define _hash_h

#define HASH_BYTES 20
#define HASH_BITS (HASH_BYTES * 8)

#include <dht/random.h>

typedef struct DhtHash {
    char value[HASH_BYTES];	/* Network byte order */
} DhtHash;

DhtHash *DhtHash_Clone(DhtHash *hash);
void DhtHash_Destroy(DhtHash *hash);

int DhtHash_Prefix(DhtHash *hash, DhtHash *prefix, unsigned int prefix_len);
int DhtHash_Random(RandomState *rs, DhtHash *hash);
int DhtHash_PrefixedRandom(RandomState *rs, DhtHash *hash, DhtHash *prefix, int prefix_len);
void DhtHash_Invert(DhtHash *hash);

int DhtHash_Equals(DhtHash *a, DhtHash *b);
int DhtHash_SharedPrefix(DhtHash *a, DhtHash *b);

const char *DhtHash_Str(DhtHash *hash);

typedef DhtHash DhtDistance;

#define DhtDistance_Destroy(D) DhtHash_Destroy((D))

DhtDistance DhtHash_Distance(DhtHash *a, DhtHash *b);

int DhtDistance_Compare(DhtDistance *a, DhtDistance *b);

uint32_t DhtHash_Hash(DhtHash *hash);

#endif
