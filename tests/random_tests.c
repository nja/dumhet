#include "minunit.h"
#include <random.h>
#include <inttypes.h>

char *test_RandomFill()
{
    const int seed = 0;
    RandomState *rsa = RandomState_Create(seed);
    mu_assert(rsa != NULL, "RandomState_Create failed");
    
    RandomState *rsb = RandomState_Create(seed);
    mu_assert(rsb != NULL, "RandomState_Create failed");

    int64_t ia = 0, ib = 0;
    int rc = 0, count = 8;

    while (count-- > 0)
    {
	rc = Random_Fill(rsa, (char *)&ia, 8);
	mu_assert(rc == 0, "RandomFill failed");

	rc = Random_Fill(rsb, (char *)&ib, 8);

	mu_assert(ia == ib, "Random values not equal");
    }

    int i = 0;
    for (i = 0; i < 10; i++)
    {
	char *buf = malloc(i);
	Random_Fill(rsa, buf, i);
	free(buf);
    }

    RandomState_Destroy(rsa);
    RandomState_Destroy(rsb);

    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_RandomFill);

    return NULL;
}

RUN_TESTS(all_tests);
