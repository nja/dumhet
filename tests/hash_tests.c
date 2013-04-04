#include "minunit.h"
#include <dht/hash.h>

char *test_Hash_Clone()
{
    Hash *orig = malloc(sizeof(Hash));

    int i = 0;
    for (i = 0; i < HASH_BYTES; i++)
    {
	orig->value[i] = i;
    }

    Hash *clone = Hash_Clone(orig);
    mu_assert(clone != NULL, "Hash_Clone failed");
    mu_assert(clone != orig, "Not cloned");

    int cmp = Distance_Compare(orig, clone);
    mu_assert(cmp == 0, "Clone different from original");

    Hash_Destroy(orig);
    Hash_Destroy(clone);

    return NULL;
}

char *test_Hash_Equals()
{
    Hash a = {{ 0 }};
    Hash b = a;

    mu_assert(Hash_Equals(&a, &a), "Should be equal");
    mu_assert(Hash_Equals(&a, &b), "Should be equal");
    mu_assert(Hash_Equals(&b, &a), "Should be equal");

    Hash c = a;
    c.value[0] = 1;

    mu_assert(!Hash_Equals(&a, &c), "Should not be equal");
    mu_assert(!Hash_Equals(&c, &a), "Should not be equal");

    Hash d = a;
    d.value[HASH_BYTES - 1] = 0x70;

    mu_assert(!Hash_Equals(&a, &d), "Should not be equal");
    mu_assert(!Hash_Equals(&d, &a), "Should not be equal");

    return NULL;
}

char *test_Hash_Invert()
{
    Hash hash = {{ 0 }};

    Hash_Invert(&hash);

    int i = 0;
    for (i = 0; i < HASH_BYTES; i++)
    {
        mu_assert(hash.value[i] == ~0, "Invert failed");
    }

    return NULL;
}

char *test_Hash_Prefix()
{
    Hash prefix = {{ "1234567890qwertyuio" }};
    Hash inv = prefix;

    Hash_Invert(&inv);

    int i = 0;
    for (i = 0; i <= HASH_BITS; i++)
    {
	Hash result = inv;

        int rc = Hash_Prefix(&result, &prefix, i);
        mu_assert(rc == 0, "Hash_Prefix failed");

	int shared = Hash_SharedPrefix(&result, &prefix);
	mu_assert(shared == i, "Wrong prefix");
    }

    return NULL;
}

char *test_Hash_PrefixedRandom()
{
    Hash prefix = {{ 0 }};
    
    int i = 0;
    for (i = 0; i < HASH_BYTES; i++)
    {
	prefix.value[i] = i;
    }

    RandomState *rs = RandomState_Create(0);

    for (i = 0; i <= HASH_BITS; i++)
    {
	Hash random = {{ 0 }};

        int rc = Hash_PrefixedRandom(rs, &random, &prefix, i);
        mu_assert(rc == 0, "Hash_PrefixedRandom failed");

	int shared = Hash_SharedPrefix(&prefix, &random);

	mu_assert(shared >= i, "Wrong prefix");
    }

    RandomState_Destroy(rs);
    return NULL;
}

char *test_Hash_Distance()
{
    Hash ha = {{0}}, hb = {{0}}, hc = {{0}};
    Distance zero_d = {{0}};

    Distance daa = Hash_Distance(&ha, &ha);

    mu_assert(Distance_Compare(&zero_d, &daa) == 0, "Wrong distance");

    hb.value[0] = 0xE0;
    hc.value[0] = 0xF0;

    Distance dab = Hash_Distance(&ha, &hb);
    Distance dac = Hash_Distance(&ha, &hc);

    mu_assert(Distance_Compare(&daa, &dab) < 0, "Wrong distance");
    mu_assert(Distance_Compare(&dab, &dac) < 0, "Wrong distance");

    return NULL;
}

char *test_Hash_Str()
{
    Hash id = {{ 0 }};
    const int expected_len = HASH_BYTES * 2;

    int len = strlen(Hash_Str(&id));
    mu_assert(len == expected_len, "Wrong length string from Hash_Str");

    Hash_Invert(&id);

    len = strlen(Hash_Str(&id));
    mu_assert(len == expected_len, "Wrong length string from Hash_Str");

    RandomState *rs = RandomState_Create(0);
    int rc = Hash_Random(rs, &id);
    mu_assert(rc == 0, "Hash_Random failed");
    len = strlen(Hash_Str(&id));
    mu_assert(len == expected_len, "Wrong length string from Hash_Str");

    debug("hashrandom %s", Hash_Str(&id));

    RandomState_Destroy(rs);

    return NULL;
}

char *test_Hash_Hash()
{
    Hash id = {{ 0 }};

    uint32_t prev = Hash_Hash(&id);

    int i = 0;
    for (i = 0; i < HASH_BYTES; i++)
    {
        id.value[i] = 1 << i % 8;
        uint32_t hash = Hash_Hash(&id);
        debug("%s %x", Hash_Str(&id), hash);
        mu_assert(prev != hash, "Unchanged hash");
        prev = hash;
    }

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_Hash_Clone);
    mu_run_test(test_Hash_Equals);
    mu_run_test(test_Hash_Invert);
    mu_run_test(test_Hash_Prefix);
    mu_run_test(test_Hash_PrefixedRandom);
    mu_run_test(test_Hash_Distance);
    mu_run_test(test_Hash_Str);
    mu_run_test(test_Hash_Hash);

    return NULL;
}

RUN_TESTS(all_tests);
