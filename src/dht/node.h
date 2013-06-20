#ifndef _dht_node_h
#define _dht_node_h

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

/* Nodes are the same when their id, addr and port are equals. */
int Node_Same(Node *a, Node *b);

int Node_DestroyOp(void *ignore, Node *node);

typedef enum NodeStatus { Unknown, Good, Questionable, Bad } NodeStatus;

/* Seconds until a node without queries or replies is questionable or bad. */
#define NODE_RESPITE (15 * 60)
/* After the respite, this many pending queries means the node is bad. */
#define NODE_MAX_PENDING 2

NodeStatus Node_Status(Node *node, time_t time);

typedef int (*NodeOp)(void *context, Node *node);

#endif
