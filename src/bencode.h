#ifndef _bencode_h
#define _bencode_h

#include <stdlib.h>
#include <stdint.h>

typedef enum BType { BString, BInteger, BList, BDictionary } BType;

typedef struct BNode {
    enum BType type;
    union {
	uint8_t *string;
	long integer;
	struct BNode **nodes;
    } value;
    size_t count;
    uint8_t *data;
    size_t data_len;
} BNode;

BNode *BDecode(uint8_t *data, size_t len);
BNode *BDecode_str(char *data, size_t len);
BNode *BDecode_strlen(char *data);

#define BDecode_str(D, L) BDecode((uint8_t *)(D), (L))
#define BDecode_strlen(D) BDecode_str((D), strlen((D)))

void BNode_destroy(BNode *node);

#endif
