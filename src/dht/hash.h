#ifndef _dht_hash_h
#define _dht_hash_h

#define HASH_BYTES 20
#define HASH_BITS (HASH_BYTES * 8)

#include <dht/random.h>

typedef struct Hash {
    char value[HASH_BYTES];	/* Network byte order */
} Hash;

Hash *Hash_Clone(Hash *hash);
void Hash_Destroy(Hash *hash);

/* Modifies hash, setting the first prefix_len bits to those from
 * prefix. */
int Hash_Prefix(Hash *hash, Hash *prefix, unsigned int prefix_len);
/* Modifies hash, generating a random value. */
int Hash_Random(RandomState *rs, Hash *hash);
/* Modifies hash, setting the first prefix_len bits to those from
 * prefix, filling the rest with random values. */
int Hash_PrefixedRandom(RandomState *rs, Hash *hash, Hash *prefix, int prefix_len);
/* Modifies hash, inverting its bitwise value. */
void Hash_Invert(Hash *hash);

/* Returns 1 when a and b have the same value, 0 otherwise. */
int Hash_Equals(Hash *a, Hash *b);
/* Returns the number of bits, from the most significant, thare are
 * equal in a and b. */
int Hash_SharedPrefix(Hash *a, Hash *b);

/* Renders the hash value as a hexadecimal string to a static buffer
 * and returns it. */
const char *Hash_Str(Hash *hash);

/* The distance of two hashes are the bitwise XOR of their values. */
typedef Hash Distance;

#define Distance_Destroy(D) Hash_Destroy((D))

/* Returns the distance of the two hashes. */
Distance Hash_Distance(Hash *a, Hash *b);

/* Returns an integer less than, equal to, or greater than zero if a
 * is found, respectively, to be less than, to match, or be greater
 * than b. */
int Distance_Compare(Distance *a, Distance *b);

/* Computes a uint32_t hash from the DHT hash value. */
uint32_t Hash_Hash(Hash *hash);

#endif
