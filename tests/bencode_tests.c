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
	snprintf((char *)string, buffer_size, "'i%lde'", values[i]);
	size_t len = strlen((char *)string) - 2;

	BNode *node = BDecode(string + 1, buffer_size - 1);
	mu_assert(node != NULL, "BDecode failed");
	mu_assert(node->type == BInteger, "Wrong BType");
	mu_assert(node->value.integer == values[i], "Wrong integer value");
	mu_assert(node->count == 1, "Wrong count");
	mu_assert(node->data == string + 1, "Wrong data pointer");
	mu_assert(node->data_len = len, "Wrong len");

	BNode_Destroy(node);
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
	BNode *node = BDecode_strlen(strings[i]);
	mu_assert(node == NULL, "Decoded undelimited integer without error");
    }

    return NULL;
}

char *test_integer_overflow()
{
    long long overflows[] = { LONG_MAX, LLONG_MIN };
    const int overflows_len = 2;
    int i = 0;

    for (i = 0; i < overflows_len; i++)
    {
	const int string_max_len = 32;
	char string[string_max_len];
	snprintf(string, string_max_len, "i%lld0e", overflows[i]);

	BNode *node = BDecode_str(string, string_max_len);
	mu_assert(node == NULL, "Decoded overflowing integer without error");
    }

    return NULL;
}

char *test_negative_zero()
{
    char *negative_zero = "i-0e";

    BNode *node = BDecode_strlen(negative_zero);
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
	    snprintf(string, buffer_size, "'i0%lde'", values[i]);
	else if (val == LONG_MIN)
	{
	    snprintf(string, buffer_size, "'i-0%lde'", LONG_MAX);
	}
	else
	{
	    snprintf(string, buffer_size, "'i-0%lde'", labs(val));
	}
	
	BNode *node = BDecode_str(string + 1, buffer_size - 1);
	mu_assert(node == NULL, "Decoded zero padded integer without error");
    }
    
    return NULL;
}

char *test_decode_list()
{
    char *lists[] = {"le", "li0ee", "li0eli1ei2eleee",
		     "li0ei1ei3ee",
                     "li0ei1ei2ei3ee",
		     "li0ei1ei2ei3ei4ee",
		     "li0ei1ei2ei3ei4ei5ee",
		     "li0ei1ei2ei3ei4ei5ei6ee",
		     "li0ei1ei2ei3ei4ei5ei6ei7ee",
		     "li0ei1ei2ei3ei4ei5ei6ei7ei8ee",
		     "li0ei1ei2ei3ei4ei5ei6ei7ei8ei9ee"};
    const size_t lists_len = 10;
    size_t i = 0;

    for (i = 0; i < lists_len; i++)
    {
	size_t len = strlen(lists[i]);
	BNode *node = BDecode_str(lists[i], len);
	mu_assert(node != NULL, "BDecode failed");
	mu_assert(node->type == BList, "Wrong BType");
	mu_assert(node->count == i, "Wrong count");
	mu_assert(node->data == lists[i], "Wrong data pointer");
	mu_assert(node->data_len == len, "Wrong len");

	BNode_Destroy(node);
    }

    return NULL;
}

char *test_bad_lists()
{
    char *lists[] = {"l", "lle", "li00ee", "li0e", "li0eli1eeli0ei01eee"};
    const size_t lists_len = 5;
    size_t i = 0;

    for (i = 0; i < lists_len; i++)
    {
	BNode *node = BDecode_strlen(lists[i]);
	mu_assert(node == NULL, "Decoded bad list without error");
    }

    return NULL;
}

char *test_decode_string()
{
    char *strings[] = {"", "a", "123456789012345", "abcdefghijklmnopq@#$%^&*()!"};
    const size_t strings_len = 4, buffer_size = 128;
    size_t i = 0;
    char buffer[buffer_size];

    for (i = 0; i < strings_len; i++)
    {
	size_t len = strlen(strings[i]);
	snprintf(buffer, buffer_size, "%zu:%s", len, strings[i]);

	BNode *node = BDecode_str(buffer, buffer_size);
	mu_assert(node != NULL, "BDecode failed");
	mu_assert(node->type == BString, "Wrong type");
	mu_assert(strncmp(strings[i], (char *)node->value.string, len) == 0, "Wrong string");
	mu_assert(node->count == len, "Wrong count");
	mu_assert(node->data == buffer, "Wrong data pointer");
	mu_assert(node->data_len == strlen(buffer), "Wrong data len");

	BNode_Destroy(node);
    }

    return NULL;
}

char *test_bad_strings()
{
    char *strings[] = {"1:", "2:a", "3abcd", "03:abc"};
    const size_t strings_len = 4;
    size_t i = 0;
    
    for (i = 0; i < strings_len; i++)
    {
	BNode *node = BDecode_strlen(strings[i]);
	mu_assert(node == NULL, "Decoded bad string without error");
    }

    return NULL;
}

char *test_decode_dictionary()
{
    char *dicts[] = {"de", "d1:b2:bae", "d0:li1ei11ee2:xxd3:vvvi-3e3:xxxi-3eee",
		     "d1:\0010:1:\1760:1:\1770:e"};
    const size_t dicts_len = 4;
    size_t i = 0;

    for (i = 0; i < dicts_len; i++)
    {
	BNode *node = BDecode_strlen(dicts[i]);
	mu_assert(node != NULL, "BDecode failed");
	mu_assert(node->type == BDictionary, "Wrong BType");
	mu_assert(node->count == i * 2, "Wrong count");
	mu_assert(node->data == dicts[i], "Wrong data pointer");
	mu_assert(node->data_len = strlen(dicts[i]), "Wrong data_len");

	BNode_Destroy(node);
    }

    return NULL;
}

char *test_bad_dictionaries()
{
    char *dicts[] = {"d2:ab0:2:cde", "d1:zi1e2:ai2ee", "d1:a0:2:bb0:"};
    const size_t dicts_len = 3;
    size_t i = 0;

    for (i = 0; i < dicts_len; i++)
    {
	BNode *node = BDecode_strlen(dicts[i]);
	mu_assert(node == NULL, "Decoded bad dictionary without error");
    }

    return NULL;
}

char *test_BNode_GetValue()
{
    const int len = 16;

    char *d5 = "d1:ai0e2:aai1e2:azi2e3:bbbi3e1:zi4ee",
	*d4 = "d1:ai0e2:aai1e2:azi2e3:bbbi3ee",
	*d3 = "d1:ai0e2:aai1e2:azi2ee",
	*d2 = "d1:ai0e2:azi2ee",
	*d1 = "d1:ai0ee",
	*d0 = "de";

    char *dicts[] = {d5, d5, d5, d5, d5, d5,
		     d4, d4, d4, d4, d4,
		     d3, d3, d3, d3,
		     d2, d2, d2,
		     d1, d1,
		     d0};	
    char *keys[] = {"a", "aa", "az", "bbb", "z", "bbbb",
		    "a", "aa", "az", "bbb", "bb",
		    "a", "aa", "az", "x",
		    "a", "az", "",
		    "a", "x",
		    "foo"};
    int values[] = {0, 1, 2, 3, 4, -1,
		    0, 1, 2, 3, -1,
		    0, 1, 2, -1,
		    0, 2, -1,
		    0, -1,
		    -1};

    int i = 0;
    for (i = 0; i < len; i++)
    {
	BNode *dict = BDecode_strlen(dicts[i]);
	mu_assert(dict != NULL, "BDecode failed");
	mu_assert(dict->type == BDictionary, "Not dict");

	BNode *value = BNode_GetValue(dict, keys[i], strlen(keys[i]));

	if (values[i] >= 0)
	{
	    mu_assert(value != NULL, "BNode_GetValue failed");

	    mu_assert(value->type == BInteger, "Value not an int");
	    mu_assert(value->value.integer == values[i], "Wrong value");
	}
	else
	{
	    mu_assert(value == NULL, "BNode_GetValue should have failed");
	}
	
	BNode_Destroy(dict);
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

    mu_run_test(test_decode_list);
    mu_run_test(test_bad_lists);

    mu_run_test(test_decode_string);
    mu_run_test(test_bad_strings);

    mu_run_test(test_decode_dictionary);
    mu_run_test(test_bad_dictionaries);

    mu_run_test(test_BNode_GetValue);
   
    return NULL;
}

RUN_TESTS(all_tests);

