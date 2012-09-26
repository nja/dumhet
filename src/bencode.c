#include <dbg.h>
#include <bencode.h>
#include <ctype.h>
#include <assert.h>

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

BNode **add_to_list(BNode *node, BNode **nodes, size_t *count, size_t *capacity)
{
    const size_t InitialCapacity = 8;

    if (*count == *capacity)
    {
	assert(*capacity > 0 || nodes == NULL);

	size_t new_capacity = *capacity > 0 ? *capacity * 2 : InitialCapacity;
	BNode **new_nodes = realloc(nodes, new_capacity * sizeof(BNode *));
	check_mem(new_nodes);

	memset(new_nodes + *capacity,
	       0,
	       (new_capacity - *capacity) * sizeof(BNode *));

	nodes = new_nodes;
	*capacity = new_capacity;
    }

    nodes[(*count)++] = node;
    return nodes;
error:
    free(nodes);
    return NULL;
}

BNode *BDecode_list(char *data, size_t len)
{
    BNode **nodes = NULL;
    size_t count = 0, capacity = 0, i = 0;

    check(len > 0, "Not enough data for a list");
    check(*data == 'l', "Bad list start");

    char *next = data + 1;
    size_t next_len = len - 1;

    while (next_len > 0 && *next != 'e')
    {
	BNode *node = BDecode(next, next_len);
	check(node != NULL, "Decoding list node failed");

	nodes = add_to_list(node, nodes, &count, &capacity);
	check(node != NULL, "Growing list failed");

	next += node->data_len;
	next_len -= node->data_len;
    }

    check(*next == 'e', "Non-terminated list");

    BNode *list = BNode_create(BList);
    check_mem(list);

    list->value.nodes = nodes;
    list->count = count;
    list->data = data;
    list->data_len = next - data + 1;

    return list;

error:
    for (i = 0; i < count; i++)
    {
	BNode_destroy(nodes[i]);
    }
	
    free(nodes);

    return NULL;
}

char *find_string_length_end(char *data, size_t len)
{
    size_t i = 0;

    if (len <= i || !isdigit(data[i]))
    {
	return NULL;
    }

    for (i = 1; i < len; i++)
    {
	if (data[i] == ':')
	    return &data[i];

	if(!isdigit(data[i]))
	    break;
    }

    return NULL;
}

BNode *BDecode_string(char *data, size_t len)
{
    char *expected_length_end = find_string_length_end(data, len);
    check(expected_length_end != NULL, "Bad string length");

    char *length_end = NULL;
    long string_len = strtol(data, &length_end, 10);
    char *string = length_end + 1,
	*string_end = string + string_len;

    check(length_end == expected_length_end, "Bad string length");
    check(errno == 0, "Bad string length");
    check(string_len >= 0, "Bad string length");
    check(string_end <= data + len, "String overflows data len");

    if (string_len > 0)
	check(*data != '0', "Zero padded string length");

    BNode *node = BNode_create(BString);
    check_mem(node);

    node->value.string = string;
    node->count = string_len;
    node->data = data;
    node->data_len = string_end - data;

    return node;
error:
    return NULL;
}

BNode *BDecode(char *data, size_t len)
{
    BNode *node = NULL;

    check(data != NULL, "NULL data");

    if (len == 0)
	return NULL;

    switch (*data)
    {
    case 'i': 
	node = BDecode_integer(data, len);
	break;
    case 'l':
	node = BDecode_list(data, len);
	break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
	node = BDecode_string(data, len);
	break;
    default:
	log_err("Bad bencode start byte: 0x%02X", *data);
    }	

    return node;

error:
    if (node)
	BNode_destroy(node);

    return NULL;
}

void BNode_destroy(BNode *node)
{
    if (node == NULL)
	return;

    if (node->type == BList)
    {
	size_t i = 0;
	for (i = 0; i < node->count; i++)
	{
	    BNode_destroy(node->value.nodes[i]);
	}
	
	free(node->value.nodes);
    }

    free(node);
}
