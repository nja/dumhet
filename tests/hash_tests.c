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

    DhtDistance *daa = DhtHash_Distance(&ha, &ha);

    mu_assert(DhtDistance_Compare(&zero_d, daa) == 0, "Wrong distance");

    hb.value[0] = 0xE0;
    hc.value[0] = 0xF0;

    DhtDistance *dab = DhtHash_Distance(&ha, &hb);
    DhtDistance *dac = DhtHash_Distance(&ha, &hc);

    mu_assert(DhtDistance_Compare(daa, dab) < 0, "Wrong distance");
    mu_assert(DhtDistance_Compare(dab, dac) < 0, "Wrong distance");

    DhtDistance_Destroy(daa);
    DhtDistance_Destroy(dab);
    DhtDistance_Destroy(dac);

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_DhtHash_Clone);
    mu_run_test(test_DhtHash_Invert);
    mu_run_test(test_DhtHash_Prefix);
    mu_run_test(test_DhtHash_PrefixedRandom);
    mu_run_test(test_DhtHash_Distance);

    return NULL;
}

RUN_TESTS(all_tests);
