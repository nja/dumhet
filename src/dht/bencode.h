#ifndef _bencode_h
#define _bencode_h

#include <stdint.h>
#include <stdlib.h>

#include <lcthw/bstrlib.h>

typedef enum BType { BString, BInteger, BList, BDictionary } BType;

char *BType_Name(BType type);

typedef struct BNode {
    enum BType type;
    union {
	char *string;
	long integer;
	struct BNode **nodes;
    } value;
    size_t count;
    char *data;
    size_t data_len;
} BNode;

BNode *BDecode(char *data, size_t len);
BNode *BDecode_str(char *data, size_t len);
BNode *BDecode_strlen(char *data);

BNode *BNode_GetValue(BNode *dict, char *key, size_t key_len);

#define BDecode_str(D, L) BDecode((D), (L))
#define BDecode_strlen(D) BDecode_str((D), strlen((D)))

void BNode_Destroy(BNode *node);

char *BNode_CopyString(BNode *string);
int BNode_StringEquals(char *string, BNode *bstring);

bstring BNode_bstring(BNode *string);

#endif
