#include <assert.h>

#include <dht/hash.h>
#include <lcthw/dbg.h>

/* DhtHash */

DhtHash *DhtHash_Clone(DhtHash *hash)
{
    assert(hash != NULL && "NULL DhtHash hash pointer");

    DhtHash *clone = malloc(sizeof(DhtHash));
    check_mem(clone);

    int i = 0;
    for (i = 0; i < HASH_BYTES; i++)
    {
	clone->value[i] = hash->value[i];
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

int DhtHash_Prefix(DhtHash *hash, DhtHash *prefix, int prefix_len)
{
    assert(hash != NULL && "NULL DhtHash hash pointer");
    assert(prefix != NULL && "NULL DhtHash prefix pointer");

    check(0 <= prefix_len && prefix_len <= HASH_BITS, "Bad prefix_len");

    int i = 0;

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
    assert(HASH_BYTES % sizeof(int) == 0 && "Size confusion");

    if (a == b)
        return 1;

    int *ai = (int *)a->value, *bi = (int *)b->value;

    while (ai < (int *)&a->value[HASH_BYTES])
    {
        if (*ai++ != *bi++)
            return 0;
    }

    return 1;
}

int DhtHash_SharedPrefix(DhtHash *a, DhtHash *b)
{
    int hi = 0, bi = 0;

    for (hi = 0; hi < HASH_BYTES; hi++)
    {
	if (a->value[hi] == b->value[hi])
	{
	    bi += 8;
	    continue;
	}

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

    int *i;    
    for (i = (int *)hash->value; (char *)i < hash->value + HASH_BYTES; i++)
    {
        *i = ~*i;
    }
}

/* DhtDistance */

DhtDistance DhtHash_Distance(DhtHash *a, DhtHash *b)
{
    assert(a != NULL && b != NULL && "NULL DhtHash pointer");

    DhtDistance distance;

    int i = 0;

    for (i = 0; i < HASH_BYTES; i++)
    {
	distance.value[i] = a->value[i] ^ b->value[i];
    }

    return distance;
}

int DhtDistance_Compare(DhtDistance *a, DhtDistance *b)
{
    assert(a != NULL && b != NULL && "NULL DhtDistance pointer");

    int i = 0;

    for (i = 0; i < HASH_BYTES; i++)
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
