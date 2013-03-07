#ifndef _node_h
#define _node_h

#include <netinet/in.h>

#include <dht/hash.h>

typedef struct DhtNode {
    DhtHash id;
    struct in_addr addr;
    uint16_t port;              /* network byte order */
    time_t reply_time;
    time_t query_time;
    int pending_queries;
} DhtNode;

DhtNode *DhtNode_Create(DhtHash *id);
void DhtNode_Destroy(DhtNode *node);
void DhtNode_DestroyBlock(DhtNode *node, size_t count);

typedef enum DhtNodeStatus { Unknown, Good, Questionable, Bad } DhtNodeStatus;

#define NODE_RESPITE (15 * 60)
#define NODE_MAX_PENDING 2

DhtNodeStatus DhtNode_Status(DhtNode *node, time_t time);

#endif
