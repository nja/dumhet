#ifndef _bencode_h
#define _bencode_h

#include <stdlib.h>

typedef enum BType { BString, BInteger, BList, BDictionary } BType;

typedef struct BKeyVal {
    char *key;
    struct BNode *value;
    size_t key_len;
} BKeyVal;

typedef struct BNode {
    enum BType type;
    union {
	char *string;
	long integer;
	struct BNode **list;
	BKeyVal **dictionary;
    } value;
    size_t count;
    char *data;
    size_t data_len;
} BNode;

BNode *BDecode(char *data, size_t len);

void BNode_destroy(BNode *node);

#endif
