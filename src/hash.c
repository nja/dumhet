#include <assert.h>

#include <dht/hash.h>
#include <lcthw/dbg.h>

/* DhtHash */

DhtHash *DhtHash_Clone(DhtHash *hash)
{
    assert(hash != NULL && "NULL DhtHash hash pointer");
    assert(HASH_BYTES % sizeof(int32_t) == 0 && "Size confusion");

    DhtHash *clone = malloc(sizeof(DhtHash));
    check_mem(clone);

    unsigned int i = 0;

    for (i = 0; i <= HASH_BYTES - sizeof(int_fast32_t); i += sizeof(int_fast32_t))
    {
        *(int_fast32_t *)&clone->value[i] = *(int_fast32_t *)&hash->value[i];
    }

    for (; i <= HASH_BYTES - sizeof(int32_t); i += sizeof(int32_t))
    {
        *(int32_t *)&clone->value[i] = *(int32_t *)&hash->value[i];
    }

    return clone;
error:
    return NULL;
}

void DhtHash_Destroy(DhtHash *hash)
{
    free(hash);
}

int DhtHash_Random(RandomState *rs, DhtHash *hash)
{
    assert(rs != NULL && "NULL RandomState pointer");
    assert(hash != NULL && "NULL DhtHash pointer");

    int rc = Random_Fill(rs, (char *)hash->value, HASH_BYTES);
    check(rc == 0, "Random_Fill failed");

    return 0;
error:
    return -1;
}

int DhtHash_Prefix(DhtHash *hash, DhtHash *prefix, unsigned int prefix_len)
{
    assert(hash != NULL && "NULL DhtHash hash pointer");
    assert(prefix != NULL && "NULL DhtHash prefix pointer");

    check(prefix_len <= HASH_BITS, "Bad prefix_len");

    unsigned int i = 0;

    while (prefix_len >= sizeof(int_fast32_t) * 8)
    {
        *(int_fast32_t *)&hash->value[i] = *(int_fast32_t *)&prefix->value[i];
        prefix_len -= sizeof(int_fast32_t) * 8;
        i += sizeof(int_fast32_t);
    }

    while (prefix_len >= sizeof(int32_t) * 8)
    {
        *(int32_t *)&hash->value[i] = *(int32_t *)&prefix->value[i];
        prefix_len -= sizeof(int32_t) * 8;
        i += sizeof(int32_t);
    }

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

int DhtHash_PrefixedRandom(RandomState *rs, DhtHash *hash, DhtHash *prefix, int prefix_len)
{
    assert(rs != NULL && "NULL RandomState pointer");
    assert(hash != NULL && "NULL DhtHash hash pointer");
    assert(prefix != NULL && "NULL DhtHash prefix pointer");

    check(0 <= prefix_len && prefix_len <= HASH_BITS, "Bad prefix_len");

    int rc = DhtHash_Random(rs, hash);
    check(rc == 0, "DhtHash_Random failed");

    rc = DhtHash_Prefix(hash, prefix, prefix_len);
    check(rc == 0, "DhtHash_Prefix failed");

    return 0;
error:
    return -1;
}

int DhtHash_Equals(DhtHash *a, DhtHash *b)
{
    assert(a != NULL && "NULL DhtHash pointer");
    assert(b != NULL && "NULL DhtHash pointer");
    assert(HASH_BYTES % sizeof(int32_t) == 0 && "Size confusion");

    if (a == b)
        return 1;

    unsigned int i = 0;

    for (i = 0; i <= HASH_BYTES - sizeof(int_fast32_t); i += sizeof(int_fast32_t))
    {
        if (*(int_fast32_t *)&a->value[i] != *(int_fast32_t *)&b->value[i])
            return 0;
    }

    for (; i <= HASH_BYTES - sizeof(int32_t); i += sizeof(int32_t))
    {
        if (*(int32_t *)&a->value[i] != *(int32_t *)&b->value[i])
            return 0;
    }

    return 1;
}

int DhtHash_SharedPrefix(DhtHash *a, DhtHash *b)
{
    unsigned int hi = 0;

    for (hi = 0; hi <= HASH_BYTES - sizeof(int_fast32_t); hi += sizeof(int_fast32_t))
    {
        if (*(int_fast32_t *)&a->value[hi] != *(int_fast32_t *)&b->value[hi])
            break;
    }

    for (; hi <= HASH_BYTES - sizeof(int32_t); hi += sizeof(int32_t))
    {
        if (*(int32_t *)&a->value[hi] != *(int32_t *)&b->value[hi])
            break;
    }

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

done:
    return bi;
}

void DhtHash_Invert(DhtHash *hash)
{
    assert(HASH_BYTES % sizeof(int) == 0 && "Byte confusion");
    assert(HASH_BYTES % sizeof(int32_t) == 0 && "Size confusion");

    unsigned int i = 0;

    for (i = 0; i <= HASH_BYTES - sizeof(int_fast32_t); i += sizeof(int_fast32_t))
    {
        *(int_fast32_t *)&hash->value[i] = ~(*(int_fast32_t *)&hash->value[i]);
    }

    for (; i <= HASH_BYTES - sizeof(int32_t); i += sizeof(int32_t))
    {
        *(int32_t *)&hash->value[i] = ~(*(int32_t *)&hash->value[i]);
    }
}

/* DhtDistance */

DhtDistance DhtHash_Distance(DhtHash *a, DhtHash *b)
{
    assert(a != NULL && b != NULL && "NULL DhtHash pointer");

    DhtDistance distance;

    unsigned int i = 0;

    for (i = 0; i <= HASH_BYTES - sizeof(int_fast32_t); i += sizeof(int_fast32_t))
    {
        *(int_fast32_t *)&distance.value[i] =
            *(int_fast32_t *)&a->value[i] ^ *(int_fast32_t *)&b->value[i];
    }

    for (; i <= HASH_BYTES - sizeof(int32_t); i += sizeof(int32_t))
    {
        *(int32_t *)&distance.value[i] =
            *(int32_t *)&a->value[i] ^ *(int32_t *)&b->value[i];
    }

    return distance;
}

int DhtDistance_Compare(DhtDistance *a, DhtDistance *b)
{
    assert(a != NULL && b != NULL && "NULL DhtDistance pointer");

    unsigned int i = 0;

    for (i = 0; i < HASH_BYTES - sizeof(int_fast32_t); i += sizeof(int_fast32_t))
    {
        if (*(int_fast32_t *)&a->value[i] != *(int_fast32_t *)&b->value[i])
            break;
    }

    for (; i < HASH_BYTES; i++)
    {
	if ((unsigned char)a->value[i] < (unsigned char)b->value[i])
	    return -1;

	if ((unsigned char)b->value[i] < (unsigned char)a->value[i])
	    return 1;
    }

    return 0;
}

#define STRBUFLEN (HASH_BYTES * 2 + 1)
static char strbuf[STRBUFLEN];

const char *DhtHash_Str(DhtHash *hash)
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

    *++dst = '\0';

    return strbuf;
}

uint32_t DhtHash_Hash(void *dht)
{
    assert(dht != NULL && "NULL DhtHash pointer");
    assert(HASH_BYTES % sizeof(uint32_t) == 0 && "Size confusion");

    uint32_t *data = (uint32_t *)((DhtHash *)dht)->value;
    uint32_t *end = (uint32_t *)(((DhtHash *)dht)->value + HASH_BYTES);
    uint32_t hash = 0;

    while (data < end);
    {
        hash = hash * 33 + *data++;
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}
