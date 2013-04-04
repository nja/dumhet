#ifndef _hash_h
#define _hash_h

#define HASH_BYTES 20
#define HASH_BITS (HASH_BYTES * 8)

#include <dht/random.h>

typedef struct Hash {
    char value[HASH_BYTES];	/* Network byte order */
} Hash;

Hash *Hash_Clone(Hash *hash);
void Hash_Destroy(Hash *hash);

int Hash_Prefix(Hash *hash, Hash *prefix, unsigned int prefix_len);
int Hash_Random(RandomState *rs, Hash *hash);
int Hash_PrefixedRandom(RandomState *rs, Hash *hash, Hash *prefix, int prefix_len);
void Hash_Invert(Hash *hash);

int Hash_Equals(Hash *a, Hash *b);
int Hash_SharedPrefix(Hash *a, Hash *b);

const char *Hash_Str(Hash *hash);

typedef Hash Distance;

#define Distance_Destroy(D) Hash_Destroy((D))

Distance Hash_Distance(Hash *a, Hash *b);

int Distance_Compare(Distance *a, Distance *b);

uint32_t Hash_Hash(Hash *hash);

#endif
