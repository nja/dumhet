#include <dbg.h>
#include <bencode.h>
#include <ctype.h>

BNode *BNode_create(BType type)
{
    BNode *node = calloc(1, sizeof(BNode));

    check_mem(node);

    node->type = type;

    return node;
error:
    return NULL;
}

char *find_integer_end(char *data, size_t len)
{
    size_t i = 0;

    if (len <= i || data[i] != 'i')
    {
	return NULL;
    }

    for (i = 1; i < len; i++)
    {
	if (i > 1 && data[i] == 'e')
	    return &data[i];

	if (isdigit(data[i])
	    || (i == 1 && data[i] == '-'))
	    continue;

	break;
    }

    return NULL;
}

BNode *BDecode_integer(char *data, size_t len)
{
    char *expected_end = find_integer_end(data, len);
    check(expected_end != NULL, "Integer not properly delimited");

    char *end = NULL;
    long integer = strtol(data + 1, &end, 10);

    check(end == expected_end, "Bad integer");
    check(errno == 0, "Bad integer");

    if (integer == 0)
    {
	check(data[1] != '-', "Negative zero");
	check(data[1] == '0', "Mystery zero result");
	check(&data[2] == end, "Padded zero");
    }
    else if (integer > 0)
    {
	check(data[1] != '0', "Zero-padded positive integer");
    }
    else if (integer < 0)
    {
	check(data[1] == '-', "Mystery negative result");
	check(data[2] != '0', "Zero-padded negative integer");
    }

    BNode *node = BNode_create(BInteger);
    check_mem(node);

    node->value.integer = integer;
    node->count = 1;
    node->data = data;
    node->data_len = end - data + 1;

    return node;
error:
    return NULL;
}    

BNode *BDecode(char *data, size_t len)
{
    BNode *node = NULL;

    check(data != NULL, "NULL data");

    node = BDecode_integer(data, len);

    return node;

error:
    if (node)
	BNode_destroy(node);

    return NULL;
}

void BNode_destroy(BNode *node)
{
    free(node);
}
