#include "minunit.h"
#include <dht/hash.h>

char *test_DhtHash_Clone()
{
    DhtHash *orig = malloc(sizeof(DhtHash));

    int i = 0;
    for (i = 0; i < HASH_BYTES; i++)
    {
	orig->value[i] = i;
    }

    DhtHash *clone = DhtHash_Clone(orig);
    mu_assert(clone != NULL, "DhtHash_Clone failed");
    mu_assert(clone != orig, "Not cloned");

    int cmp = DhtDistance_Compare(orig, clone);
    mu_assert(cmp == 0, "Clone different from original");

    DhtHash_Destroy(orig);
    DhtHash_Destroy(clone);

    return NULL;
}

char *test_DhtHash_Equals()
{
    DhtHash a = {{ 0 }};
    DhtHash b = a;

    mu_assert(DhtHash_Equals(&a, &a), "Should be equal");
    mu_assert(DhtHash_Equals(&a, &b), "Should be equal");
    mu_assert(DhtHash_Equals(&b, &a), "Should be equal");

    DhtHash c = a;
    c.value[0] = 1;

    mu_assert(!DhtHash_Equals(&a, &c), "Should not be equal");
    mu_assert(!DhtHash_Equals(&c, &a), "Should not be equal");

    DhtHash d = a;
    d.value[HASH_BYTES - 1] = 0x70;

    mu_assert(!DhtHash_Equals(&a, &d), "Should not be equal");
    mu_assert(!DhtHash_Equals(&d, &a), "Should not be equal");

    return NULL;
}

char *test_DhtHash_Invert()
{
    DhtHash hash = {{ 0 }};

    DhtHash_Invert(&hash);

    int i = 0;
    for (i = 0; i < HASH_BYTES; i++)
    {
        mu_assert(hash.value[i] == ~0, "Invert failed");
    }

    return NULL;
}

char *test_DhtHash_Prefix()
{
    DhtHash prefix = {{ "1234567890qwertyuio" }};
    DhtHash inv = prefix;

    DhtHash_Invert(&inv);

    int i = 0;
    for (i = 0; i <= HASH_BITS; i++)
    {
	DhtHash result = inv;

        int rc = DhtHash_Prefix(&result, &prefix, i);
        mu_assert(rc == 0, "DhtHash_Prefix failed");

	int shared = DhtHash_SharedPrefix(&result, &prefix);
	mu_assert(shared == i, "Wrong prefix");
    }

    return NULL;
}

char *test_DhtHash_PrefixedRandom()
{
    DhtHash prefix = {{ 0 }};
    
    int i = 0;
    for (i = 0; i < HASH_BYTES; i++)
    {
	prefix.value[i] = i;
    }

    RandomState *rs = RandomState_Create(0);

    for (i = 0; i <= HASH_BITS; i++)
    {
	DhtHash random = {{ 0 }};

        int rc = DhtHash_PrefixedRandom(rs, &random, &prefix, i);
        mu_assert(rc == 0, "DhtHash_PrefixedRandom failed");

	int shared = DhtHash_SharedPrefix(&prefix, &random);

	mu_assert(shared >= i, "Wrong prefix");
    }

    RandomState_Destroy(rs);
    return NULL;
}

char *test_DhtHash_Distance()
{
    DhtHash ha = {{0}}, hb = {{0}}, hc = {{0}};
    DhtDistance zero_d = {{0}};

    DhtDistance daa = DhtHash_Distance(&ha, &ha);

    mu_assert(DhtDistance_Compare(&zero_d, &daa) == 0, "Wrong distance");

    hb.value[0] = 0xE0;
    hc.value[0] = 0xF0;

    DhtDistance dab = DhtHash_Distance(&ha, &hb);
    DhtDistance dac = DhtHash_Distance(&ha, &hc);

    mu_assert(DhtDistance_Compare(&daa, &dab) < 0, "Wrong distance");
    mu_assert(DhtDistance_Compare(&dab, &dac) < 0, "Wrong distance");

    return NULL;
}

char *test_DhtHash_Str()
{
    DhtHash id = {{ 0 }};
    const int expected_len = HASH_BYTES * 2;

    int len = strlen(DhtHash_Str(&id));
    mu_assert(len == expected_len, "Wrong length string from DhtHash_Str");

    DhtHash_Invert(&id);

    len = strlen(DhtHash_Str(&id));
    mu_assert(len == expected_len, "Wrong length string from DhtHash_Str");

    RandomState *rs = RandomState_Create(0);
    int rc = DhtHash_Random(rs, &id);
    mu_assert(rc == 0, "DhtHash_Random failed");
    len = strlen(DhtHash_Str(&id));
    mu_assert(len == expected_len, "Wrong length string from DhtHash_Str");

    debug("hashrandom %s", DhtHash_Str(&id));

    RandomState_Destroy(rs);

    return NULL;
}

char *test_DhtHash_Hash()
{
    DhtHash id = {{ 0 }};

    uint32_t prev = DhtHash_Hash(&id);

    int i = 0;
    for (i = 0; i < HASH_BYTES; i++)
    {
        id.value[i] = 1 << i % 8;
        uint32_t hash = DhtHash_Hash(&id);
        debug("%s %x", DhtHash_Str(&id), hash);
        mu_assert(prev != hash, "Unchanged hash");
        prev = hash;
    }

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_DhtHash_Clone);
    mu_run_test(test_DhtHash_Equals);
    mu_run_test(test_DhtHash_Invert);
    mu_run_test(test_DhtHash_Prefix);
    mu_run_test(test_DhtHash_PrefixedRandom);
    mu_run_test(test_DhtHash_Distance);
    mu_run_test(test_DhtHash_Str);
    mu_run_test(test_DhtHash_Hash);

    return NULL;
}

RUN_TESTS(all_tests);
