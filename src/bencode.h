#ifndef _bencode_h
#define _bencode_h

#include <stdlib.h>
#include <stdint.h>

typedef enum BType { BString, BInteger, BList, BDictionary } BType;

char *BType_Name(BType type);

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

BNode *BNode_GetValue(BNode *dict, uint8_t *key, size_t key_len);

#define BDecode_str(D, L) BDecode((uint8_t *)(D), (L))
#define BDecode_strlen(D) BDecode_str((D), strlen((D)))

void BNode_destroy(BNode *node);

#endif
