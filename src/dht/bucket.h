#ifndef _dht_bucket_h
#define _dht_bucket_h

#include <time.h>

#include <lcthw/darray.h>
#include <dht/node.h>

#define BUCKET_K 8
#define BUCKET_LAST_BITS 3

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

Node *Bucket_ReplaceBad(Bucket *bucket, Node *node);
Node *Bucket_ReplaceQuestionable(Bucket *bucket, Node *node);

int Bucket_AddNode(Bucket *bucket, Node *node);

int Bucket_GatherGoodNodes(Bucket *bucket, DArray *found);

#endif
