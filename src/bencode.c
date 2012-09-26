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

uint8_t *find_integer_end(uint8_t *data, size_t len)
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

BNode *BDecode_integer(uint8_t *data, size_t len)
{
    uint8_t *expected_end = find_integer_end(data, len);
    check(expected_end != NULL, "Integer not properly delimited");

    uint8_t *end = NULL;
    long integer = strtol((char *)data + 1, (char **)&end, 10);

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
    return NULL;
}

BNode *BDecode_list_or_dict(uint8_t *data, size_t len, char head_ch, BType type)
{
    BNode *node = NULL;
    BNode **nodes = NULL;
    size_t count = 0, capacity = 0, i = 0;

    check(len > 0, "Not enough data for a list");
    check(*data == head_ch, "Bad list start");

    uint8_t *next = data + 1;
    size_t next_len = len - 1;

    while (next_len > 0 && *next != 'e')
    {
	node = BDecode(next, next_len);
	check(node != NULL, "Decoding list node failed");

	BNode **new_nodes = add_to_list(node, nodes, &count, &capacity);
	check(new_nodes != NULL, "Growing list failed");
	nodes = new_nodes;

	next += node->data_len;
	next_len -= node->data_len;
    }

    check(*next == 'e', "Non-terminated list");

    BNode *result = BNode_create(type);
    check_mem(result);

    result->value.nodes = nodes;
    result->count = count;
    result->data = data;
    result->data_len = next - data + 1;

    return result;

error:
    for (i = 0; i < count; i++)
    {
	BNode_destroy(nodes[i]);

	if (node == nodes[i])
	    node = NULL;
    }

    BNode_destroy(node);
	
    free(nodes);

    return NULL;
}

BNode *BDecode_list(uint8_t *data, size_t len)
{
    return BDecode_list_or_dict(data, len, 'l', BList);
}

uint8_t *find_string_length_end(uint8_t *data, size_t len)
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

BNode *BDecode_string(uint8_t *data, size_t len)
{
    uint8_t *expected_length_end = find_string_length_end(data, len);
    check(expected_length_end != NULL, "Bad string length");

    uint8_t *length_end = NULL;
    long string_len = strtol((char *)data, (char **)&length_end, 10);
    uint8_t *string = length_end + 1,
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

int is_less_than(uint8_t *a, const size_t a_len, uint8_t *b, const size_t b_len)
{
    size_t i = 0, min_len = a_len < b_len ? a_len : b_len;

    for (i = 0; i < min_len; i++)
    {
	if (a[i] < b[i])
	    return 1;

	if (b[i] < a[i])
	    return 0;
    }

    return a_len < b_len;
}

int all_string_keys(BNode *nodes, size_t count)
{
    size_t i = 0;

    for (i = 0; i < count; i += 2)
    {
	if (nodes[i].type != BString)
	    return 0;
    }

    return 1;
}

BNode *BDecode_dictionary(uint8_t *data, size_t len)
{
    BNode *node = BDecode_list_or_dict(data, len, 'd', BDictionary);
    check(node != NULL, "List decoding for dictionary failed");
    check(node->count % 2 == 0, "Odd number of dict list nodes");

    size_t i = 0;

    for (i = 0; i < node->count; i += 2)
    {
	BNode *cur = node->value.nodes[i];
	check(cur->type == BString, "Non-string dictionary key");
	
	if (i == 0) continue;
	
	BNode *prev = node->value.nodes[i-2];

	check(is_less_than(prev->value.string, prev->count,
			   cur->value.string, cur->count),
	      "Dictionary keys not sorted");
    }

    return node;
error:

    BNode_destroy(node);

    return NULL;
}

BNode *BDecode(uint8_t *data, size_t len)
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
    case 'd':
	node = BDecode_dictionary(data, len);
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

    if (node->type == BList || node->type == BDictionary)
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
