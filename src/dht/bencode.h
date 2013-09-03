#ifndef _dht_bencode_h
#define _dht_bencode_h

#include <stdint.h>
#include <stdlib.h>

#include <lcthw/bstrlib.h>

/* Types of the dedcoded objects. */
typedef enum BType { BString, BInteger, BList, BDictionary } BType;

/* A node in a tree of decoded objects with pointers to the relevant
 * parts in the source data buffer. */
typedef struct BNode {
    enum BType type;
    union {
	char *string;
	long integer;
	struct BNode **nodes;
    } value;
    size_t count;               /* Length of string or number of nodes. */
    char *data;                 /* Original source data of this node. */
    size_t data_len;            /* Length of source data. */
} BNode;

/* Decodes the bencoded data into a tree of nodes with pointers into
 * the data buffer. */
BNode *BDecode(char *data, size_t len);

/* From a BDictionary, get the value of key. */
BNode *BNode_GetValue(BNode *dict, char *key, size_t key_len);

#define BDecode_str(D, L) BDecode((D), (L))
#define BDecode_strlen(D) BDecode_str((D), strlen((D)))

void BNode_Destroy(BNode *node);

/* Copy the BString value to a new string. */
char *BNode_CopyString(BNode *string);
int BNode_StringEquals(char *string, BNode *bstring);

/* Copy the BString value to a bstring. */
bstring BNode_bstring(BNode *string);

#endif
