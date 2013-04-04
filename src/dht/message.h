#ifndef _dht_message_h
#define _dht_message_h

#include <dht/node.h>
#include <lcthw/bstrlib.h>

typedef struct Peer {
    uint32_t addr;
    uint16_t port;
} Peer;

typedef enum MessageType {
    QPing, QFindNode, QGetPeers, QAnnouncePeer,
    RPing, RFindNode, RGetPeers, RAnnouncePeer,
    RError    
} MessageType;

int MessageType_IsQuery(MessageType type);

typedef struct QPingData {
} QPingData;

typedef struct QFindNodeData {
    Hash *target;
} QFindNodeData;

typedef struct QGetPeersData {
    Hash *info_hash;
} QGetPeersData;

typedef struct QAnnouncePeerData {
    Hash *info_hash;
    uint16_t port;
    char *token;
    size_t token_len;
} QAnnouncePeerData;

typedef struct RPingData {
} RPingData;

typedef struct RFindNodeData {
    Node **nodes;
    size_t count;
} RFindNodeData;

/* May be cast as RFindNodeData */
typedef struct RGetPeersData {
    Node **nodes;
    size_t count;
    char *token;
    size_t token_len;
    Peer *values;
} RGetPeersData;

typedef struct RAnnouncePeerData {
} RAnnouncePeerData;

typedef struct RErrorData {
    int code;
    bstring message;
} RErrorData;

typedef struct Message {
    MessageType type;
    Node node;
    char *t;
    size_t t_len;
    Hash id;
    void *context;
    union {
	QPingData qping;
	QFindNodeData qfindnode;
	QGetPeersData qgetpeers;
	QAnnouncePeerData qannouncepeer;
	RPingData rping;
	RFindNodeData rfindnode;
	RGetPeersData rgetpeers;
	RAnnouncePeerData rannouncepeer;
	RErrorData rerror;
    } data;
} Message;

void Message_Destroy(Message *message);
void Message_DestroyNodes(Message *message);

#define RERROR_GENERIC       201
#define RERROR_SERVER        202
#define RERROR_PROTOCOL      203
#define RERROR_METHODUNKNOWN 204

#endif
