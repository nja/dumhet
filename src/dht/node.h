#ifndef _node_h
#define _node_h

#include <netinet/in.h>

#include <dht/hash.h>

typedef struct Node {
    Hash id;
    struct in_addr addr;
    uint16_t port;              /* network byte order */
    time_t reply_time;
    time_t query_time;
    int pending_queries;
} Node;

Node *Node_Create(Hash *id);
Node *Node_Copy(Node *node);
void Node_Destroy(Node *node);
void Node_DestroyBlock(Node **node, size_t count);

int Node_Same(Node *a, Node *b);

int Node_DestroyOp(void *ignore, Node *node);

typedef enum NodeStatus { Unknown, Good, Questionable, Bad } NodeStatus;

#define NODE_RESPITE (15 * 60)
#define NODE_MAX_PENDING 2

NodeStatus Node_Status(Node *node, time_t time);

typedef int (*NodeOp)(void *context, Node *node);

#endif
