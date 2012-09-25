#include "minunit.h"
#include <bencode.h>
#include <limits.h>

static long values[] = {0, 1, -1, 23,
			SHRT_MIN, SHRT_MAX,
			INT_MIN, INT_MAX,
			LONG_MIN, LONG_MAX};
static const int values_len = 10;

char *test_decode_integer()
{
    const int buffer_size = 128;
    char string[buffer_size];
    int i = 0;

    for (i = 0; i < values_len; i++)
    {
	sprintf(string, "'i%lde'", values[i]);
	size_t len = strlen(string) - 2;

	debug("Integer: %s", string);

	BNode *node = BDecode(string + 1, buffer_size - 1);
	mu_assert(node != NULL, "BDecode failed");
	mu_assert(node->type == BInteger, "Wrong BType");
	mu_assert(node->value.integer == values[i], "Wrong integer value");
	mu_assert(node->count == 1, "Wrong count");
	mu_assert(node->data == string + 1, "Wrong data pointer");
	mu_assert(node->data_len = len, "Wrong len");

	BNode_destroy(node);
    }
    
    return NULL;
}

char *test_undelimited_integers()
{
    char *strings[] = { "", "213", "i234", "e", "ie", "234e" };
    const int strings_len = 6;
    int i = 0;
    
    for (i = 0; i < strings_len; i++)
    {
	debug("Undelimited integer: '%s'", strings[i]);

	size_t len = strlen(strings[i]);

	BNode *node = BDecode(strings[i], len);
	mu_assert(node == NULL, "Decoded undelimited integer without error");
    }

    return NULL;
}

char *test_integer_overflow()
{
    long overflows[] = { LONG_MAX, LLONG_MIN };
    const int overflows_len = 2;
    int i = 0;

    for (i = 0; i < overflows_len; i++)
    {
	const int string_max_len = 32;
	char string[string_max_len];
	sprintf(string, "i%ld0e", overflows[i]);

	debug("Overflow integer: '%s'", string);

	BNode *node = BDecode(string, string_max_len);
	mu_assert(node == NULL, "Decoded overflowing integer without error");
    }

    return NULL;
}

char *test_negative_zero()
{
    char *negative_zero = "i-0e";
    size_t len = strlen(negative_zero);

    BNode *node = BDecode(negative_zero, len);
    mu_assert(node == NULL, "Decoded negative zero without error");

    return NULL;
}

char *test_zero_padded_integers()
{
    const int buffer_size = 128;
    char string[buffer_size];
    int i = 0;

    for (i = 0; i < values_len; i++)
    {
	long val = values[i];

	if (val >= 0)
	    sprintf(string, "'i0%lde'", values[i]);
	else if (val == LONG_MIN)
	{
	    sprintf(string, "'i-0%lde'", LONG_MAX);
	}
	else
	{
	    sprintf(string, "'i-0%lde'", labs(val));
	}
	
	debug("Integer: %s", string);

	BNode *node = BDecode(string + 1, buffer_size - 1);
	mu_assert(node == NULL, "Decoded zero padded integer without error");
    }
    
    return NULL;
}

char *all_tests()
{
    mu_suite_start();

    mu_run_test(test_decode_integer);
    mu_run_test(test_undelimited_integers);
    mu_run_test(test_integer_overflow);
    mu_run_test(test_negative_zero);
    mu_run_test(test_zero_padded_integers);
    
    return NULL;
}

RUN_TESTS(all_tests);

