#ifndef _dht_bucket_h
#define _dht_bucket_h

#include <time.h>

#include <lcthw/darray.h>
#include <dht/node.h>

#define BUCKET_K 8
/* The last bits worth of hash ids will all fit in the same bucket. */
#define BUCKET_LAST_BITS 3

/* A Bucket holds up to BUCKET_K Nodes.
 * The Nodes of a Bucket share at least index bits of id prefix.
 * The nodes array may be sparse. */
typedef struct Bucket {
    int index;
    int count;
    time_t change_time;
    Node *nodes[BUCKET_K];
} Bucket;

Bucket *Bucket_Create();
void Bucket_Destroy(Bucket *bucket);

int Bucket_ContainsNode(Bucket *bucket, Node *node);
int Bucket_IsFull(Bucket *bucket);

/* Returns the replaced node, or NULL when no Bad was found */
Node *Bucket_ReplaceBad(Bucket *bucket, Node *node);
/* Returns the replaced node, or NULL when no Questionable was found */
Node *Bucket_ReplaceQuestionable(Bucket *bucket, Node *node);

int Bucket_AddNode(Bucket *bucket, Node *node);

#endif
