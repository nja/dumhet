#ifndef _dht_table_h
#define _dht_table_h

#include <dht/bucket.h>
#include <dht/hash.h>
#include <dht/node.h>
#include <lcthw/bstrlib.h>

#define MAX_TABLE_BUCKETS (HASH_BITS + 1 - BUCKET_LAST_BITS)

/* Holds buckets of nodes of decreasing distance to id.
 * Nodes in bucket[i] share a prefix of i bits with id.*/
typedef struct Table {
    Hash id;
    Bucket *buckets[MAX_TABLE_BUCKETS];
    int end;
} Table;

Table *Table_Create(Hash *id);
void Table_Destroy(Table *table);
void Table_DestroyNodes(Table *table);

/* Calls op(context, node) for each node of the table. */
int Table_ForEachNode(Table *table, void *context, NodeOp op);

Bucket *Table_AddBucket(Table *table);
Bucket *Table_FindBucket(Table *table, Hash *id);

Node *Table_FindNode(Table *table, Hash *id);

enum Table_InsertNodeResultRc {
    ERROR,                      /* Insert failed. */
    OKAdded,                    /* Added to .bucket */
    OKReplaced,                 /* Replaced .replaced in .bucket */
    OKFull,                     /* Target bucket full. */
    OKAlreadyAdded              /* Node of same id already added. */
};

/* Returns a new array with the BUCKET_K nodes from table that are
 * closest to the id. Returns NULL on error. */
DArray *Table_GatherClosest(Table *table, Hash *id);

typedef struct Table_InsertNodeResult {
    enum Table_InsertNodeResultRc rc;
    Bucket *bucket;             /* The target bucket (OKAdded,
                                 * OKReplaced, OKAlreadyAdded). */
    Node *replaced;             /* The replaced node (OKReplaced). */
} Table_InsertNodeResult;

Table_InsertNodeResult Table_InsertNode(Table *table, Node *node);

/* Fins the node with the given id in the table and updates its
 * reply_time.*/
int Table_MarkReply(Table *table, Hash *id);
/* Fins the node with the given id in the table and updates its
 * query_time.*/
void Table_MarkQuery(Table *table, Hash *id);

bstring Table_Dump(Table *table);
Table *Table_Read(bstring dump);

#endif
