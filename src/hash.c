#include <assert.h>
#include <string.h>

#include <dht/hash.h>
#include <lcthw/dbg.h>

/* Hash */

Hash *Hash_Clone(Hash *hash)
{
    assert(hash != NULL && "NULL Hash hash pointer");
    assert(HASH_BYTES % sizeof(int32_t) == 0 && "Size confusion");

    Hash *clone = malloc(sizeof(Hash));
    check_mem(clone);

    memcpy(clone->value, hash->value, HASH_BYTES);

    return clone;
error:
    return NULL;
}

void Hash_Destroy(Hash *hash)
{
    free(hash);
}

int Hash_Random(RandomState *rs, Hash *hash)
{
    assert(rs != NULL && "NULL RandomState pointer");
    assert(hash != NULL && "NULL Hash pointer");

    int rc = Random_Fill(rs, (char *)hash->value, HASH_BYTES);
    check(rc == 0, "Random_Fill failed");

    return 0;
error:
    return -1;
}

int Hash_Prefix(Hash *hash, Hash *prefix, unsigned int prefix_len)
{
    assert(hash != NULL && "NULL Hash hash pointer");
    assert(prefix != NULL && "NULL Hash prefix pointer");

    check(prefix_len <= HASH_BITS, "Bad prefix_len");

    unsigned int i = 0;

    while (prefix_len >= 8)
    {
	hash->value[i] = prefix->value[i];

	prefix_len -= 8;
	i++;
    }

    if (i < HASH_BYTES)
    {
	char mask = ((char)~0) << (8 - prefix_len);
	hash->value[i] &= ~mask;
	hash->value[i] |= mask & prefix->value[i];
    }

    return 0;
error:
    return -1;
}

int Hash_PrefixedRandom(RandomState *rs, Hash *hash, Hash *prefix, int prefix_len)
{
    assert(rs != NULL && "NULL RandomState pointer");
    assert(hash != NULL && "NULL Hash hash pointer");
    assert(prefix != NULL && "NULL Hash prefix pointer");

    check(0 <= prefix_len && prefix_len <= HASH_BITS, "Bad prefix_len");

    int rc = Hash_Random(rs, hash);
    check(rc == 0, "Hash_Random failed");

    rc = Hash_Prefix(hash, prefix, prefix_len);
    check(rc == 0, "Hash_Prefix failed");

    return 0;
error:
    return -1;
}

int Hash_Equals(Hash *a, Hash *b)
{
    assert(a != NULL && "NULL Hash pointer");
    assert(b != NULL && "NULL Hash pointer");

    return memcmp(a->value, b->value, HASH_BYTES) == 0;
}

int Hash_SharedPrefix(Hash *a, Hash *b)
{
    unsigned int hi = 0;

    for (; hi < HASH_BYTES; hi++)
    {
	if (a->value[hi] != b->value[hi])
            break;
    }

    unsigned int bi = hi * 8;

    if (hi < HASH_BYTES)
    {
	uint8_t mask = 1 << 7;
        uint8_t xor = a->value[hi] ^ b->value[hi];
	while (mask)
	{
            if (!(mask & xor))
	    {
		bi++;
		mask = mask >> 1;
	    }
	    else
	    {
		goto done;
	    }
	}
    }

    assert(bi <= HASH_BITS && "Shared prefix too large");

done:
    return bi;
}

void Hash_Invert(Hash *hash)
{
    assert(HASH_BYTES % sizeof(int) == 0 && "Byte confusion");
    assert(HASH_BYTES % sizeof(int32_t) == 0 && "Size confusion");

    unsigned int i = 0;

    for (i = 0; i < HASH_BYTES; i++)
    {
        hash->value[i] = ~hash->value[i];
    }
}

/* Distance */

Distance Hash_Distance(Hash *a, Hash *b)
{
    assert(a != NULL && b != NULL && "NULL Hash pointer");

    Distance distance;

    int i = 0;

    for (i = 0; i < HASH_BYTES; i++)
    {
        distance.value[i] = a->value[i] ^ b->value[i];
    }

    return distance;
}

int Distance_Compare(Distance *a, Distance *b)
{
    assert(a != NULL && b != NULL && "NULL Distance pointer");

    return memcmp(a->value, b->value, HASH_BYTES);
}

#define HASHSTRLEN (HASH_BYTES * 2)
#define STRBUFLEN (HASHSTRLEN + 1)
static char strbuf[STRBUFLEN];

const char *Hash_Str(Hash *hash)
{
    char *src = hash->value;
    char *dst = strbuf;
    char *hex = "0123456789abcdef";

    while (src < hash->value + HASH_BYTES && dst + 1 < strbuf + STRBUFLEN)
    {
        int high = (unsigned char)*src >> 4;
        int low = *src & 0x0f;
        *dst++ = hex[high];
        *dst++ = hex[low];
        src++;
    }

    strbuf[HASHSTRLEN] = '\0';

    return strbuf;
}

uint32_t Hash_Hash(Hash *dht)
{
    assert(dht != NULL && "NULL Hash pointer");
    assert(HASH_BYTES % sizeof(uint32_t) == 0 && "Size confusion");

    uint32_t *data = (uint32_t *)dht->value;
    uint32_t *end = data + HASH_BYTES / sizeof(uint32_t);
    uint32_t hash = 0;

    while (data < end)
    {
        hash += *data++;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}
